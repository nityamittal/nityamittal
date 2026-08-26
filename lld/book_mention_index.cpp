// Book management system with a character mention index -- the core version,
// sized for a 30-minute coding window. Covers F1-F6 only.
//
// Build: g++ -std=c++17 -O2 -Wall book_mention_index.cpp -o book_mention_index
//
// Two ideas carry the whole solution:
//   1. Tokenize each line once. Word boundaries, case, and punctuation are all
//      handled by "split on non-alphanumeric, then lowercase" -- so "Harry's"
//      matches and "Harrying" does not, with no special cases.
//   2. Count while indexing, not while querying. mentionCount() is a hash
//      lookup: O(1), independent of book length.
//
// See book_mention_index_extended.cpp for mutation, co-occurrence, late
// character registration, multi-book counts, and locking.

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Occurrence {
    int chapter;
    int line;    // 0-based, within the chapter
    int offset;  // byte offset, within the line
};

// "Harry's wand" -> (harry,0) (s,6) (wand,8)
// "\"HARRY!\""   -> (harry,1)
// "Harrying"     -> (harrying,0)
vector<pair<string, int>> tokenize(const string& line) {
    vector<pair<string, int>> tokens;
    for (int i = 0; i < (int)line.size();) {
        if (!isalnum((unsigned char)line[i])) {
            ++i;
            continue;
        }
        int start = i;
        string word;
        while (i < (int)line.size() && isalnum((unsigned char)line[i]))
            word += tolower((unsigned char)line[i++]);
        tokens.push_back({word, start});
    }
    return tokens;
}

class Book {
public:
    Book(string title, const vector<string>& characters) : title_(title) {
        for (const string& name : characters) {
            isCharacter_[normalize(name)] = name;  // normalized form -> canonical form
            total_[name] = 0;
        }
    }

    const string& title() const { return title_; }

    // F1 -- add a chapter and index it in one pass. O(total characters).
    void addChapter(const string& chapterTitle, const vector<string>& lines) {
        int chapter = (int)chapterTitles_.size();
        chapterTitles_.push_back(chapterTitle);
        for (int i = 0; i < (int)lines.size(); ++i) {
            for (const auto& [word, offset] : tokenize(lines[i])) {
                auto it = isCharacter_.find(word);
                if (it == isCharacter_.end()) continue;
                const string& name = it->second;
                ++total_[name];
                ++byChapter_[name][chapter];
                occurrences_[name].push_back({chapter, i, offset});
            }
        }
    }

    // F3 -- O(1).
    int mentionCount(const string& name) const {
        auto it = total_.find(name);
        return it == total_.end() ? 0 : it->second;
    }

    // F4 -- O(1).
    int mentionCount(const string& name, int chapter) const {
        auto it = byChapter_.find(name);
        if (it == byChapter_.end()) return 0;
        auto jt = it->second.find(chapter);
        return jt == it->second.end() ? 0 : jt->second;
    }

    // F5 -- already in reading order, because indexing walked the book in order.
    vector<Occurrence> occurrences(const string& name) const {
        auto it = occurrences_.find(name);
        return it == occurrences_.end() ? vector<Occurrence>{} : it->second;
    }

    // F6 -- O(C log C) over a small character set.
    vector<pair<string, int>> topCharacters(int k) const {
        vector<pair<string, int>> all(total_.begin(), total_.end());
        sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
            return a.second != b.second ? a.second > b.second : a.first < b.first;
        });
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

private:
    static string normalize(const string& name) {
        vector<pair<string, int>> tokens = tokenize(name);
        return tokens.size() == 1 ? tokens[0].first : string{};  // single-word names only
    }

    string title_;
    vector<string> chapterTitles_;
    unordered_map<string, string> isCharacter_;
    unordered_map<string, int> total_;
    unordered_map<string, map<int, int>> byChapter_;
    unordered_map<string, vector<Occurrence>> occurrences_;
};

// ------------------------------------------------------------------- Driver

int main() {
    Book book("Harry Potter and the Philosopher's Stone", {"Harry", "Ron", "Draco"});

    book.addChapter("The Boy Who Lived",
                    {"Harry Potter was a wizard.",
                     "\"HARRY, HARRY!\" shouted Ron from the doorway.",
                     "Harrying the Muggles was not something Harry enjoyed.",
                     "Draco sneered at Harry's broomstick."});
    book.addChapter("The Potions Master",
                    {"Ron and Draco traded insults across the dungeon.",
                     "Snape ignored them both.",
                     "\"Ten points from Gryffindor,\" he said to Ron."});

    // F3 -- "HARRY" counts, "Harry's" counts, "Harrying" does not.
    assert(book.mentionCount("Harry") == 5);
    assert(book.mentionCount("Ron") == 3);
    assert(book.mentionCount("Draco") == 2);
    assert(book.mentionCount("Snape") == 0);  // never registered

    // F4 -- per chapter.
    assert(book.mentionCount("Harry", 0) == 5);
    assert(book.mentionCount("Harry", 1) == 0);
    assert(book.mentionCount("Ron", 1) == 2);

    // F5 -- positions, in reading order.
    vector<Occurrence> harry = book.occurrences("Harry");
    assert(harry.size() == 5);
    assert(harry[0].chapter == 0 && harry[0].line == 0 && harry[0].offset == 0);
    assert(harry[1].line == 1 && harry[2].line == 1);  // both shouts, same line
    assert(harry[3].line == 2);                        // "Harrying" skipped
    assert(harry[4].line == 3);                        // "Harry's" kept

    // F6 -- top-k.
    auto top = book.topCharacters(2);
    assert(top.size() == 2);
    assert(top[0].first == "Harry" && top[0].second == 5);
    assert(top[1].first == "Ron" && top[1].second == 3);

    cout << "All assertions passed.\n\n";
    for (const auto& [name, n] : book.topCharacters(10)) cout << name << ": " << n << "\n";
    return 0;
}
