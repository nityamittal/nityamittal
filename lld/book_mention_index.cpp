// Book management system with a character mention index.
//
// Build: g++ -std=c++17 -O2 -Wall book_mention_index.cpp -o book_mention_index
//
// Design notes:
//   * Counts are maintained incrementally on every write, so mentionCount() is
//     O(1) and never depends on book length.
//   * Positional data (occurrence lists, per-character line sets) is derived
//     state. Writes mark the book dirty; the next positional query materializes
//     it in one ordered walk. This keeps edits O(length of edited line) without
//     paying to keep reading-order lists sorted after every insert/delete.
//   * Matching lives entirely in the Tokenizer: normalization and word
//     boundaries fall out of "split on non-word characters, then lowercase".

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace library {

using CharacterId = int;
using ChapterId   = int;
using BookId      = std::string;

// ---------------------------------------------------------------- Tokenizer

struct Token {
    std::string text;  // normalized (lowercased)
    int start;         // byte offset of the token in the original line
    int length;        // byte length in the original line
};

class Tokenizer {
public:
    virtual ~Tokenizer() = default;
    virtual std::vector<Token> tokenize(const std::string& line) const = 0;
};

// Splits on every non-alphanumeric byte and lowercases what is left.
//   "Harry's wand"  -> harry, s, wand        (possessive still matches)
//   "\"HARRY!\""    -> harry                 (case-insensitive)
//   "Harrying"      -> harrying              (no false positive)
// Limitation: byte-oriented, so non-ASCII names are not normalized.
class WordTokenizer : public Tokenizer {
public:
    std::vector<Token> tokenize(const std::string& line) const override {
        std::vector<Token> out;
        const int n = static_cast<int>(line.size());
        int i = 0;
        while (i < n) {
            while (i < n && !isWordChar(line[i])) ++i;
            const int start = i;
            std::string word;
            while (i < n && isWordChar(line[i])) {
                word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(line[i]))));
                ++i;
            }
            if (!word.empty()) out.push_back(Token{std::move(word), start, i - start});
        }
        return out;
    }

private:
    static bool isWordChar(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0;
    }
};

// ------------------------------------------------------------------ Domain

struct Character {
    CharacterId id;
    std::string name;  // canonical, as given
};

struct Occurrence {
    ChapterId chapter;
    int line;         // 0-based line number within the chapter, as of now
    int startOffset;  // byte offset within the line
    int length;
};

struct LinePos {
    ChapterId chapter;
    int line;
    bool operator<(const LinePos& o) const {
        return chapter != o.chapter ? chapter < o.chapter : line < o.line;
    }
    bool operator==(const LinePos& o) const {
        return chapter == o.chapter && line == o.line;
    }
};

namespace detail {

// One matched name inside a line. Position is line-relative, so it survives
// lines moving up and down within a chapter.
struct Hit {
    CharacterId cid;
    int start;
    int length;
};

struct Line {
    std::string text;
    std::vector<Hit> hits;  // sorted by start
};

struct Chapter {
    ChapterId id;
    std::string title;
    std::vector<Line> lines;
};

}  // namespace detail

// -------------------------------------------------------------- BookIndex

// Owns one book's text and its derived indexes. Thread-safe: concurrent
// readers, exclusive writers, one lock per book.
class BookIndex {
public:
    BookIndex(BookId id, std::string title, std::string author,
              std::shared_ptr<const Tokenizer> tokenizer)
        : id_(std::move(id)),
          title_(std::move(title)),
          author_(std::move(author)),
          tokenizer_(std::move(tokenizer)) {}

    const BookId& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& author() const { return author_; }

    // --- writes ---

    // Registers a character for this book. If the book already has text, only
    // the new name is scanned for; existing characters are untouched.
    void registerCharacter(const Character& c) {
        std::unique_lock lock(mu_);
        const std::string key = normalize(c.name);
        if (key.empty()) throw std::invalid_argument("character name has no word characters");
        auto [it, inserted] = nameToId_.emplace(key, c.id);
        if (!inserted) {
            if (it->second != c.id) throw std::invalid_argument("name already bound to another character");
            return;
        }
        names_[c.id] = c.name;
        total_.emplace(c.id, 0);
        for (auto& ch : chapters_)
            for (auto& ln : ch.lines) indexOneName(ch.id, ln, key, c.id);
        dirty_ = true;
    }

    void addChapter(ChapterId chapterId, std::string chapterTitle,
                    const std::vector<std::string>& lines) {
        std::unique_lock lock(mu_);
        detail::Chapter ch{chapterId, std::move(chapterTitle), {}};
        ch.lines.reserve(lines.size());
        for (const std::string& text : lines) {
            detail::Line ln{text, scan(text)};
            applyCounts(chapterId, ln.hits, +1);
            ch.lines.push_back(std::move(ln));
        }
        chapters_.push_back(std::move(ch));
        dirty_ = true;
    }

    void editLine(ChapterId chapterId, int lineNumber, const std::string& newText) {
        std::unique_lock lock(mu_);
        detail::Line& ln = lineAt(chapterId, lineNumber);
        applyCounts(chapterId, ln.hits, -1);
        ln.text = newText;
        ln.hits = scan(newText);
        applyCounts(chapterId, ln.hits, +1);
        dirty_ = true;
    }

    void insertLine(ChapterId chapterId, int lineNumber, const std::string& text) {
        std::unique_lock lock(mu_);
        detail::Chapter& ch = chapterAt(chapterId);
        if (lineNumber < 0 || lineNumber > static_cast<int>(ch.lines.size()))
            throw std::out_of_range("line number");
        detail::Line ln{text, scan(text)};
        applyCounts(chapterId, ln.hits, +1);
        ch.lines.insert(ch.lines.begin() + lineNumber, std::move(ln));
        dirty_ = true;
    }

    void deleteLine(ChapterId chapterId, int lineNumber) {
        std::unique_lock lock(mu_);
        detail::Chapter& ch = chapterAt(chapterId);
        detail::Line& ln = lineAt(chapterId, lineNumber);
        applyCounts(chapterId, ln.hits, -1);
        ch.lines.erase(ch.lines.begin() + lineNumber);
        dirty_ = true;
    }

    // --- reads ---

    // O(1).
    int count(CharacterId cid) const {
        std::shared_lock lock(mu_);
        auto it = total_.find(cid);
        return it == total_.end() ? 0 : it->second;
    }

    // O(1).
    int count(CharacterId cid, ChapterId chapterId) const {
        std::shared_lock lock(mu_);
        auto it = byChapter_.find(cid);
        if (it == byChapter_.end()) return 0;
        auto jt = it->second.find(chapterId);
        return jt == it->second.end() ? 0 : jt->second;
    }

    // O(number of results) once materialized; reading order.
    std::vector<Occurrence> occurrences(CharacterId cid) const {
        std::unique_lock lock(mu_);  // may materialize
        materialize();
        auto it = occurrences_.find(cid);
        return it == occurrences_.end() ? std::vector<Occurrence>{} : it->second;
    }

    // O(C log k) over the (small) character set.
    std::vector<std::pair<CharacterId, int>> topCharacters(int k) const {
        std::shared_lock lock(mu_);
        std::vector<std::pair<CharacterId, int>> all(total_.begin(), total_.end());
        std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
            return a.second != b.second ? a.second > b.second : a.first < b.first;
        });
        if (k >= 0 && static_cast<int>(all.size()) > k) all.resize(k);
        return all;
    }

    // Lines where every one of the given characters appears.
    // O(size of the smallest line set).
    std::vector<LinePos> linesContainingAll(const std::vector<CharacterId>& cids) const {
        std::unique_lock lock(mu_);  // may materialize
        materialize();
        if (cids.empty()) return {};
        const std::vector<LinePos>* smallest = nullptr;
        for (CharacterId cid : cids) {
            auto it = linesOf_.find(cid);
            if (it == linesOf_.end()) return {};
            if (!smallest || it->second.size() < smallest->size()) smallest = &it->second;
        }
        std::vector<LinePos> result = *smallest;
        for (CharacterId cid : cids) {
            const std::vector<LinePos>& other = linesOf_.at(cid);
            if (&other == smallest) continue;
            std::vector<LinePos> merged;
            std::set_intersection(result.begin(), result.end(), other.begin(), other.end(),
                                  std::back_inserter(merged));
            result.swap(merged);
            if (result.empty()) break;
        }
        return result;
    }

    std::string lineText(const LinePos& pos) const {
        std::shared_lock lock(mu_);
        return const_cast<BookIndex*>(this)->lineAt(pos.chapter, pos.line).text;
    }

    std::string nameOf(CharacterId cid) const {
        std::shared_lock lock(mu_);
        auto it = names_.find(cid);
        return it == names_.end() ? std::string{} : it->second;
    }

private:
    std::string normalize(const std::string& name) const {
        std::vector<Token> t = tokenizer_->tokenize(name);
        if (t.size() != 1) {
            // Single-token names only: a multi-word name would need phrase
            // matching, which this index deliberately does not do.
            if (t.empty()) return {};
            throw std::invalid_argument("character name must be a single word: " + name);
        }
        return t.front().text;
    }

    std::vector<detail::Hit> scan(const std::string& text) const {
        std::vector<detail::Hit> hits;
        for (const Token& t : tokenizer_->tokenize(text)) {
            auto it = nameToId_.find(t.text);
            if (it != nameToId_.end()) hits.push_back(detail::Hit{it->second, t.start, t.length});
        }
        return hits;  // already ordered by start
    }

    // Adds hits for a single newly registered name to an already-scanned line.
    void indexOneName(ChapterId chapterId, detail::Line& ln, const std::string& key, CharacterId cid) {
        std::vector<detail::Hit> added;
        for (const Token& t : tokenizer_->tokenize(ln.text))
            if (t.text == key) added.push_back(detail::Hit{cid, t.start, t.length});
        if (added.empty()) return;
        applyCounts(chapterId, added, +1);
        ln.hits.insert(ln.hits.end(), added.begin(), added.end());
        std::sort(ln.hits.begin(), ln.hits.end(),
                  [](const detail::Hit& a, const detail::Hit& b) { return a.start < b.start; });
    }

    void applyCounts(ChapterId chapterId, const std::vector<detail::Hit>& hits, int sign) {
        for (const detail::Hit& h : hits) {
            total_[h.cid] += sign;
            byChapter_[h.cid][chapterId] += sign;
        }
    }

    // Rebuilds reading-order positional state. Counts are never rebuilt here —
    // they are always exact.
    void materialize() const {
        if (!dirty_) return;
        occurrences_.clear();
        linesOf_.clear();
        for (const auto& [cid, _] : total_) {
            occurrences_[cid];
            linesOf_[cid];
        }
        for (const detail::Chapter& ch : chapters_) {
            for (int i = 0; i < static_cast<int>(ch.lines.size()); ++i) {
                const detail::Line& ln = ch.lines[i];
                CharacterId last = -1;
                for (const detail::Hit& h : ln.hits) {
                    occurrences_[h.cid].push_back(Occurrence{ch.id, i, h.start, h.length});
                    if (h.cid != last) {
                        std::vector<LinePos>& ls = linesOf_[h.cid];
                        LinePos p{ch.id, i};
                        if (ls.empty() || !(ls.back() == p)) ls.push_back(p);
                        last = h.cid;
                    }
                }
            }
        }
        dirty_ = false;
    }

    detail::Chapter& chapterAt(ChapterId chapterId) {
        for (auto& ch : chapters_)
            if (ch.id == chapterId) return ch;
        throw std::out_of_range("no such chapter");
    }

    detail::Line& lineAt(ChapterId chapterId, int lineNumber) {
        detail::Chapter& ch = chapterAt(chapterId);
        if (lineNumber < 0 || lineNumber >= static_cast<int>(ch.lines.size()))
            throw std::out_of_range("line number");
        return ch.lines[lineNumber];
    }

    BookId id_;
    std::string title_;
    std::string author_;
    std::shared_ptr<const Tokenizer> tokenizer_;

    std::vector<detail::Chapter> chapters_;
    std::unordered_map<std::string, CharacterId> nameToId_;  // normalized name -> id
    std::unordered_map<CharacterId, std::string> names_;

    // Always exact.
    std::unordered_map<CharacterId, int> total_;
    std::unordered_map<CharacterId, std::unordered_map<ChapterId, int>> byChapter_;

    // Derived, rebuilt lazily.
    mutable bool dirty_ = false;
    mutable std::unordered_map<CharacterId, std::vector<Occurrence>> occurrences_;
    mutable std::unordered_map<CharacterId, std::vector<LinePos>> linesOf_;

    mutable std::shared_mutex mu_;
};

// ---------------------------------------------------------- LibraryService

// Character ids are global, so the same character can be tracked across a
// series while each book keeps its own index.
class LibraryService {
public:
    explicit LibraryService(std::shared_ptr<const Tokenizer> tokenizer =
                                std::make_shared<WordTokenizer>())
        : tokenizer_(std::move(tokenizer)) {}

    CharacterId characterId(const std::string& name) {
        auto it = characterIds_.find(name);
        if (it != characterIds_.end()) return it->second;
        CharacterId id = nextCharacterId_++;
        characterIds_.emplace(name, id);
        return id;
    }

    void addBook(const BookId& bookId, const std::string& title, const std::string& author,
                 const std::vector<std::string>& characterNames,
                 const std::vector<std::pair<std::string, std::vector<std::string>>>& chapters) {
        auto book = std::make_unique<BookIndex>(bookId, title, author, tokenizer_);
        for (const std::string& n : characterNames) book->registerCharacter(Character{characterId(n), n});
        ChapterId cid = 0;
        for (const auto& [chapterTitle, lines] : chapters) book->addChapter(cid++, chapterTitle, lines);
        books_[bookId] = std::move(book);
    }

    BookIndex& book(const BookId& bookId) {
        auto it = books_.find(bookId);
        if (it == books_.end()) throw std::out_of_range("no such book: " + bookId);
        return *it->second;
    }
    const BookIndex& book(const BookId& bookId) const {
        return const_cast<LibraryService*>(this)->book(bookId);
    }

    CharacterId registerCharacter(const BookId& bookId, const std::string& name) {
        CharacterId id = characterId(name);
        book(bookId).registerCharacter(Character{id, name});
        return id;
    }

    int mentionCount(const BookId& bookId, CharacterId cid) const { return book(bookId).count(cid); }
    int mentionCount(const BookId& bookId, CharacterId cid, ChapterId ch) const {
        return book(bookId).count(cid, ch);
    }
    std::vector<Occurrence> occurrences(const BookId& bookId, CharacterId cid) const {
        return book(bookId).occurrences(cid);
    }

    // Across the whole library.
    int mentionCount(CharacterId cid) const {
        int total = 0;
        for (const auto& [_, b] : books_) total += b->count(cid);
        return total;
    }

private:
    std::shared_ptr<const Tokenizer> tokenizer_;
    std::unordered_map<BookId, std::unique_ptr<BookIndex>> books_;
    std::unordered_map<std::string, CharacterId> characterIds_;
    CharacterId nextCharacterId_ = 1;
};

}  // namespace library

// ------------------------------------------------------------------ Demo

using namespace library;

int main() {
    LibraryService lib;

    const std::vector<std::pair<std::string, std::vector<std::string>>> chapters = {
        {"The Boy Who Lived",
         {"Harry Potter was a wizard.",
          "\"HARRY, HARRY!\" shouted Ron from the doorway.",
          "Harrying the Muggles was not something Harry enjoyed.",
          "Draco sneered at Harry's broomstick."}},
        {"The Potions Master",
         {"Ron and Draco traded insults across the dungeon.",
          "Snape ignored them both.",
          "\"Ten points from Gryffindor,\" he said to Ron."}}};

    lib.addBook("hp1", "Harry Potter and the Philosopher's Stone", "J.K. Rowling",
                {"Harry", "Ron", "Draco"}, chapters);

    const CharacterId harry = lib.characterId("Harry");
    const CharacterId ron   = lib.characterId("Ron");
    const CharacterId draco = lib.characterId("Draco");

    // F3 -- total counts. "Harrying" must not count; "Harry's" and "HARRY" must.
    assert(lib.mentionCount("hp1", harry) == 5);
    assert(lib.mentionCount("hp1", ron) == 3);
    assert(lib.mentionCount("hp1", draco) == 2);

    // F4 -- per chapter.
    assert(lib.mentionCount("hp1", harry, 0) == 5);
    assert(lib.mentionCount("hp1", harry, 1) == 0);
    assert(lib.mentionCount("hp1", ron, 1) == 2);

    // F5 -- positions, in reading order.
    std::vector<Occurrence> ho = lib.occurrences("hp1", harry);
    assert(ho.size() == 5);
    assert(ho[0].chapter == 0 && ho[0].line == 0 && ho[0].startOffset == 0);
    assert(ho[1].line == 1 && ho[2].line == 1);  // both shouts on the same line
    assert(ho[3].line == 2);                     // "Harrying" skipped, "Harry" kept
    assert(ho[4].line == 3);

    // F6 -- top-k.
    auto top = lib.book("hp1").topCharacters(2);
    assert(top.size() == 2 && top[0].first == harry && top[0].second == 5);
    assert(top[1].first == ron && top[1].second == 3);

    // F9 -- co-occurrence.
    auto together = lib.book("hp1").linesContainingAll({ron, draco});
    assert(together.size() == 1 && together[0].chapter == 1 && together[0].line == 0);

    // F10 -- mutation. Counts stay exact; positions follow the shift.
    lib.book("hp1").insertLine(0, 1, "Ron waved at Draco.");
    assert(lib.mentionCount("hp1", ron) == 4);
    assert(lib.mentionCount("hp1", draco) == 3);
    assert(lib.occurrences("hp1", harry)[1].line == 2);  // shouts pushed down one line

    lib.book("hp1").editLine(0, 0, "Neville Longbottom was a wizard.");
    assert(lib.mentionCount("hp1", harry) == 4);

    lib.book("hp1").deleteLine(0, 1);
    assert(lib.mentionCount("hp1", ron) == 3);

    // F2 (late) -- register a character after indexing; only that name is scanned.
    const CharacterId snape = lib.registerCharacter("hp1", "Snape");
    assert(lib.mentionCount("hp1", snape) == 1);
    assert(lib.mentionCount("hp1", harry) == 4);  // untouched

    // F11 -- library-wide count.
    lib.addBook("hp2", "Chamber of Secrets", "J.K. Rowling", {"Harry", "Ron"},
                {{"The Worst Birthday", {"Harry could not sleep.", "Ron sent an owl to Harry."}}});
    assert(lib.mentionCount(harry) == 4 + 2);

    // Empty results, not exceptions.
    assert(lib.mentionCount("hp2", draco) == 0);
    assert(lib.occurrences("hp2", draco).empty());

    std::cout << "All assertions passed.\n\n";
    for (const auto& [cid, n] : lib.book("hp1").topCharacters(10))
        std::cout << lib.book("hp1").nameOf(cid) << ": " << n << "\n";
    return 0;
}
