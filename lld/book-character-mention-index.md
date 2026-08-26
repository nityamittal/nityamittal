# LLD: Book Management System with a Character Mention Index

**Format:** 45–60 min low-level design round (Amazon-style)
**Deliverable from the candidate:** class model + working code for the core index, driven by a small `main`/test harness. No database, no framework, no network.
**Reference implementation:** [`book_mention_index.cpp`](./book_mention_index.cpp)

---

## 1. Problem statement (read this to the candidate)

> We are building the backend for a digital library. A **book** is stored as an ordered list of **lines** — each line is a plain string, and lines are grouped into **chapters**.
>
> Readers and literary analysts want to ask questions about **characters**. For a book like *Harry Potter and the Philosopher's Stone*, they want to know: how many times is **Harry** mentioned? How many times **Ron**? **Draco**?
>
> Design and implement a system that ingests books, maintains a **character mention index**, and answers mention queries efficiently. Ingestion is relatively rare; queries are very frequent.

Let the candidate ask clarifying questions before writing anything. The ambiguity is deliberate — see §3.

---

## 2. Functional requirements

### Must have (core, ~25 min)

| # | Requirement |
|---|---|
| F1 | Add a book: id, title, author, and its ordered lines grouped into chapters. |
| F2 | Register the characters of a book (`Harry`, `Ron`, `Draco`), each identified by a single-word name. |
| F3 | `mentionCount(bookId, characterId)` → total number of times that character is mentioned in the book. |
| F4 | `mentionCount(bookId, characterId, chapterId)` → count scoped to one chapter. |
| F5 | `occurrences(bookId, characterId)` → the position of every mention: `(chapterId, lineNumber, startOffset)`, in reading order. |
| F6 | `topCharacters(bookId, k)` → the k most-mentioned characters, descending. |

### Should have (extensions, ~15 min — pick 2)

| # | Requirement |
|---|---|
| F7 | **Late registration.** A character is added *after* the book is indexed. Indexing must cover only that character, not rebuild the book. |
| F8 | **Co-occurrence.** `linesContainingAll(bookId, [ron, draco])` → lines where all of the given characters appear together. Used for relationship analytics. |
| F9 | **Mutability.** `editLine`, `insertLine`, `deleteLine` on an already-indexed book. Queries afterwards must reflect the edit, without re-indexing the whole book. |
| F10 | **Multi-book.** `mentionCount(characterId)` across the whole library — a character spans a series. |
| F11 | **A second index type.** Index spells or locations the same way, without touching the book/ingestion classes. |

### Explicitly out of scope

Persistence, HTTP layer, auth, result pagination, and anything NLP-grade: pronoun resolution (`he` → Harry), multi-word or nickname resolution (`The Boy Who Lived` → Harry), disambiguating two characters who share a surname. Characters are single-word names, given up front. Say this out loud if the candidate drifts there.

---

## 3. Clarifying questions the candidate is expected to ask

A strong candidate surfaces most of these **before** coding. Reveal an answer only when asked.

1. **Is matching case-sensitive?** → No. `harry`, `Harry`, and `HARRY` all count.
2. **Word boundaries?** → Yes, and this is the crux. `Harry's` and `(Harry)` and `Harry—Ron` count; `Harrying`, `Harrys`, and `Charry` do **not**.
3. **Multiple mentions on one line?** → All count. `"Harry, Harry!" said Ron.` is 2 for Harry, 1 for Ron.
4. **Are character names known up front?** → Yes at ingest, but see F7 — they can also arrive later.
5. **What if a character's name is also a common word** (`Bill`, `Sirius`, `Rose`)? → Out of scope for correctness, but a good candidate flags that the index will over-count and that disambiguation needs context the index doesn't have.
6. **Read/write ratio?** → Write-once-read-many. Precompute at ingest; queries must not scan text.
7. **Book size?** → Up to ~10⁶ lines / ~10⁷ words; characters per book < 1000; library up to 10⁵ books. One book's working set fits in memory.
8. **Concurrency?** → Concurrent readers, occasional writer. Raise it in the extension phase.

---

## 4. Non-functional requirements

- **NF1 — Query latency.** `mentionCount` must be **O(1)** — independent of book length. `occurrences` is O(number of results).
- **NF2 — Ingestion.** A single pass over the text: **O(total characters)**, not O(text × number of registered characters). Running `line.find(name)` for every character on every line is the failure mode to probe.
- **NF3 — Memory.** Storing every occurrence position is acceptable. Storing a per-character copy of the text is not.
- **NF4 — Extensibility.** Adding a second index type (spells, locations, potions) must not modify the book or ingestion classes. Look for a `Tokenizer` / `Analyzer` seam rather than `if (type == CHARACTER)`.
- **NF5 — Thread safety.** Concurrent `mentionCount` calls are safe; an in-flight edit never exposes a half-updated index.

---

## 5. Suggested interview flow

| Minutes | Phase | What to look for |
|---|---|---|
| 0–5 | Restate + clarify | Do they ask about case, boundaries, read/write ratio? |
| 5–15 | Class model on the board | Clean separation: domain (`Book`, `Chapter`, `Line`, `Character`) vs. matching (`Tokenizer`) vs. index (`BookIndex`) vs. API (`LibraryService`). |
| 15–35 | Code the core | `addBook`, index build, `mentionCount`, `occurrences`, running, with a few asserts. |
| 35–50 | Pick 2 extensions | Mutability is the richest; co-occurrence and late registration are the quickest. |
| 50–60 | Complexity, tests, trade-offs | Big-O of ingest and each query; edge cases in §7; what breaks at 100×. |

---

## 6. Expected API surface

Names may differ; the shape matters.

```cpp
using CharacterId = int;
using ChapterId   = int;
using BookId      = std::string;

struct Character  { CharacterId id; std::string name; };          // single word
struct Occurrence { ChapterId chapter; int line; int startOffset; int length; };
struct LinePos    { ChapterId chapter; int line; };

struct Token { std::string text; int start; int length; };        // normalized

class Tokenizer {                                                 // the NF4 seam
public:
    virtual std::vector<Token> tokenize(const std::string& line) const = 0;
};

class BookIndex {
public:
    void registerCharacter(const Character&);                     // F2, F7
    void addChapter(ChapterId, std::string title, const std::vector<std::string>& lines);

    int  count(CharacterId) const;                                // F3, O(1)
    int  count(CharacterId, ChapterId) const;                     // F4, O(1)
    std::vector<Occurrence> occurrences(CharacterId) const;       // F5
    std::vector<std::pair<CharacterId,int>> topCharacters(int k) const;         // F6
    std::vector<LinePos> linesContainingAll(const std::vector<CharacterId>&) const;  // F8

    void editLine(ChapterId, int lineNumber, const std::string& newText);       // F9
    void insertLine(ChapterId, int lineNumber, const std::string& text);
    void deleteLine(ChapterId, int lineNumber);
};

class LibraryService {                                            // public entry point
public:
    void addBook(const BookId&, title, author, characterNames, chapters);
    int  mentionCount(const BookId&, CharacterId) const;
    int  mentionCount(CharacterId) const;                         // F10, whole library
    BookIndex& book(const BookId&);
};
```

---

## 7. Edge cases to raise if the candidate doesn't

1. `"Harry, Harry!"` — repeats on one line.
2. `Harry's wand`, `(Harry)`, `Harry—Ron` — punctuation adjacency must still match.
3. `Harrying`, `Harrys`, `Charry` — must **not** match. This is the single most common bug.
4. `HARRY` shouted in dialogue — case-insensitive.
5. `Ron` vs `Ronald` — distinct tokens, distinct characters; no prefix matching.
6. A character registered **after** the book was indexed (F7).
7. `insertLine` shifting every downstream line number — how do stored occurrence positions stay correct?
8. `editLine` removing the last mention of a character — count must go to 0, not stay stale.
9. Empty book, or a character with zero mentions → return 0 / empty, never throw.
10. Unicode names (`Fleur`, `Krum`) — a byte-oriented `isalnum` splits accented characters; state the limitation.
11. A name appearing in the chapter *title* — indexed or not? Pick a policy and be consistent.

---

## 8. Evaluation rubric

**Strong hire**
- Domain model separated from indexing; matching lives behind a `Tokenizer` interface, so a second index type is a new dictionary, not a new `if`.
- Ingest is one tokenizing pass with a hash lookup per token → O(total characters). Explains *why* per-character substring scanning is O(text × characters) and rejects it.
- Word boundaries fall out of the tokenizer rather than being special-cased — recognizes that "split on non-word characters, then lowercase" solves case, punctuation, possessives, and the `Harrying` false positive in one stroke.
- Counts maintained incrementally on every write → `mentionCount` is genuinely O(1) even after edits.
- For F9, understands that line-relative offsets survive line shifts and absolute line numbers do not, and picks a strategy for the derived positional data (rebuild lazily, or keep stable line ids).
- Names the trade-offs: occurrence lists vs. counts only; eager vs. lazy positional state; per-book locking.

**Hire**
- Correct working core: `unordered_map<CharacterId,int>` for counts, `unordered_map<CharacterId, vector<Occurrence>>` for positions.
- Tokenizes each line once and looks tokens up in a name→id map; handles case and boundaries.
- Sensible class boundaries; can state complexities when asked.

**No hire signals**
- `mentionCount` scans the book on every call and they don't notice when asked about a 10⁶-line book.
- `strstr` / `find` per character per line, no word-boundary handling, no awareness of the `Harrying` false positive.
- Counting logic embedded in `Book`, or one 200-line god class with no seam for a second index type.
- Cannot state the complexity of their own ingest loop.
- Zero clarifying questions; codes against an invented spec.

---

## 9. Reference design (interviewer's notes — do not show the candidate)

**Structures, per book:**

```
nameToId    : unordered_map<normalized name, CharacterId>
chapters    : vector<Chapter>, Chapter = { id, title, vector<Line> }
Line        : { text, vector<Hit> }   Hit = { cid, startOffset, length }   // sorted by offset
total       : unordered_map<CharacterId, int>                      // always exact
byChapter   : unordered_map<CharacterId, unordered_map<ChapterId, int>>   // always exact
occurrences : unordered_map<CharacterId, vector<Occurrence>>       // derived, reading order
linesOf     : unordered_map<CharacterId, vector<LinePos>>          // derived, sorted
```

**Ingest** — for each line: tokenize once (split on non-alphanumeric, lowercase), look each token up in `nameToId`, and on a hit append a `Hit` to the line and bump both counters. **O(total characters)**, independent of the number of characters registered. The tokenizer, not a regex, is what gives word boundaries.

**The key insight for mutability (F9):** a `Hit` stores a *line-relative* offset, so it survives its line moving up or down. Only the derived structures (`occurrences`, `linesOf`) embed absolute line numbers. So: keep counts exact incrementally on every write (`editLine` = subtract old hits, rescan, add new hits — O(length of edited line)), mark the book dirty, and rebuild the positional structures on the next positional query in one ordered walk. Counts, the hot path, stay O(1) and always correct; positions, the rare path, are lazily materialized. A candidate who instead keeps stable `LineId`s and per-line occurrence buckets is equally right.

**`topCharacters(k)`** — sort or a size-k min-heap over `total`: O(C log k) with C < 1000. A maintained sorted structure is over-engineering; accept either if justified.

**`linesContainingAll`** — intersect the sorted `linesOf` vectors, smallest first: O(size of the smallest set).

**Late registration (F7)** — add the name to `nameToId`, then walk lines and add hits for that one name only. O(book), once, for one character — not a rebuild.

**Concurrency (NF5)** — one `shared_mutex` per book: shared for counts and top-k, exclusive for writes and for queries that may materialize. The point is that locking is scoped to a book, not the library.

**Scaling** (only if there's time) — the index is per-book, so it shards cleanly by `bookId`. Counts are a natural materialized view; occurrence lists are the expensive part and can be recomputed on demand from stored text.

---

## 10. Follow-up questions to stretch a strong candidate

1. Character names are *not* given — infer them from the text. What changes? (Capitalization and frequency heuristics, an NER pass, and now the index must be rebuildable.)
2. "How many times do Harry and Draco appear within 3 lines of each other?" — a proximity query: two-pointer merge over two sorted occurrence lists.
3. A character is renamed mid-series (`Riddle` → `Voldemort`). Merge two character indexes without re-scanning.
4. `topCharacters` across all 10⁵ books. What do you precompute, and where does it live?
5. Readers annotate books with their own private character lists. How do you avoid rebuilding a shared index per user?
6. Now support two-word names after all. What breaks, and what is the smallest change that fixes it? (Tokenizer emits n-grams, or the dictionary becomes a trie / Aho-Corasick automaton — and now overlapping matches need a longest-match rule.)
