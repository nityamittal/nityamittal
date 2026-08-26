# LLD: Book Management System with a Character Mention Index

**Format:** 45–60 min low-level design round (Amazon-style)
**Deliverable from the candidate:** class model + working code for the core index, driven by a small `main`/test harness. No database, no framework, no network.

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
| F2 | Register the characters of a book (e.g. `Harry`, `Ron`, `Draco`), each with a canonical name. |
| F3 | `mentionCount(bookId, characterId)` → total number of times that character is mentioned in the book. |
| F4 | `mentionCount(bookId, characterId, chapterId)` → count scoped to one chapter. |
| F5 | `occurrences(bookId, characterId)` → the positions of every mention: `(chapterId, lineNumber, startOffset)`, in reading order. |
| F6 | `topCharacters(bookId, k)` → the k most-mentioned characters, descending. |

### Should have (extensions, ~15 min — pick 2)

| # | Requirement |
|---|---|
| F7 | **Aliases.** A character has many surface forms: `Harry`, `Potter`, `Harry Potter`, `The Boy Who Lived`. All of them count toward the same character. Multi-word aliases must match across a single line. |
| F8 | **Longest-match, no double counting.** The line `Harry Potter waved.` is **one** mention of Harry, not three (`Harry`, `Potter`, `Harry Potter`). Overlapping aliases resolve to the longest match. |
| F9 | **Co-occurrence.** `linesWith(bookId, [harry, ron])` → lines where all of the given characters appear together. Used for "relationship" analytics. |
| F10 | **Mutability.** `editLine`, `insertLine`, `deleteLine` on an already-indexed book. Queries after the edit must reflect it, without re-indexing the whole book. |
| F11 | **Multi-book / global query.** `mentionCount(characterId)` across the whole library (a character can span a series). |

### Explicitly out of scope

Persistence, HTTP layer, auth, pagination of results, NLP-grade entity resolution (pronoun resolution, "he/she" → character, disambiguating two characters named *Weasley*). Say this out loud if the candidate drifts there.

---

## 3. Clarifying questions the candidate is expected to ask

A strong candidate surfaces most of these **before** coding. Reveal an answer only when asked.

1. **Is matching case-sensitive?** → No. `harry` and `Harry` both count.
2. **Word boundaries?** → Yes. `Harry's` and `Harry,` count; `Harrypotter` and `Harrying` do **not**. Punctuation and apostrophes must not break a match.
3. **Are character names known up front?** → Yes, characters are registered per book before/at ingestion. (Follow-up: what changes if a character is added *after* indexing? Expected answer: index only that character over the stored lines, not a full rebuild.)
4. **Multiple mentions on one line?** → All count. `"Harry, Harry!" said Ron.` is 2 for Harry, 1 for Ron.
5. **Do aliases overlap between characters?** → Assume no for the core; ask what they'd do if `Potter` were ambiguous between Harry and his father (expected: reject at registration, or attach a disambiguation policy).
6. **Read/write ratio?** → Write-once-read-many. Precompute at ingest; queries must not scan the text.
7. **Book size?** → Up to ~10⁶ lines, ~10⁷ words; number of characters per book is small (< 1000). Library up to 10⁵ books. Must fit the working set of one book in memory.
8. **Concurrency?** → Concurrent readers, occasional writer. Bring it up in the extension phase.

---

## 4. Non-functional requirements

- **NF1 — Query latency.** `mentionCount` must be O(1); it must not depend on book length. `occurrences` is O(number of results).
- **NF2 — Ingestion.** One pass over the text: O(total characters × cost per position), **not** O(characters × number of aliases). Naïvely running `line.contains(alias)` for every alias on every line is the failure mode to probe.
- **NF3 — Memory.** Storing every occurrence is acceptable; storing a per-line copy of the text per character is not.
- **NF4 — Extensibility.** Adding a new *kind* of index (spells, locations, potions) must not require modifying the book/ingestion classes. Look for an `Analyzer`/`IndexBuilder` abstraction rather than `if (type == CHARACTER)`.
- **NF5 — Thread safety.** Concurrent `mentionCount` calls are safe; an in-flight edit must not expose a partially updated index.

---

## 5. Suggested interview flow

| Minutes | Phase | What to look for |
|---|---|---|
| 0–5 | Restate + clarify | Does the candidate ask about case, boundaries, aliases, read/write ratio? |
| 5–15 | Class model on the board | Clean separation: domain (`Book`, `Chapter`, `Line`, `Character`) vs. indexing (`Tokenizer`, `IndexBuilder`, `MentionIndex`) vs. service/API (`LibraryService`). |
| 15–35 | Code the core | `addBook`, index build, `mentionCount`, `occurrences`. Compiling, runnable, with a couple of asserts. |
| 35–50 | Pick 2 extensions | Aliases + longest-match, or mutability, or co-occurrence. |
| 50–60 | Complexity, tests, trade-offs | Big-O of ingest and each query; the edge cases in §7; what breaks at 100× scale. |

---

## 6. Expected API surface

The candidate may name things differently; the shape matters, not the names.

```java
// ---- Domain ----
class Book {
    BookId id; String title; String author;
    List<Chapter> chapters;
}
class Chapter { ChapterId id; String title; List<String> lines; }

class Character {
    CharacterId id;
    String canonicalName;      // "Harry Potter"
    Set<String> aliases;       // "Harry", "Potter", "The Boy Who Lived"
}

// ---- Position of a single mention ----
record Occurrence(ChapterId chapterId, int lineNumber, int startOffset, int length) {}

// ---- Index (one per book) ----
interface MentionIndex {
    int count(CharacterId c);
    int count(CharacterId c, ChapterId chapter);
    List<Occurrence> occurrences(CharacterId c);
    List<CharacterId> topCharacters(int k);
    Set<Integer> linesContainingAll(List<CharacterId> cs);   // F9
}

// ---- Service (the public entry point) ----
interface LibraryService {
    void addBook(Book book, List<Character> characters);
    void registerCharacter(BookId bookId, Character c);      // post-hoc registration
    int  mentionCount(BookId bookId, CharacterId c);
    int  mentionCount(BookId bookId, CharacterId c, ChapterId ch);
    List<Occurrence> occurrences(BookId bookId, CharacterId c);
    List<CharacterId> topCharacters(BookId bookId, int k);

    // F10 — mutation
    void editLine(BookId bookId, ChapterId ch, int lineNumber, String newText);
    void insertLine(BookId bookId, ChapterId ch, int lineNumber, String text);
    void deleteLine(BookId bookId, ChapterId ch, int lineNumber);
}
```

---

## 7. Edge cases to raise if the candidate doesn't

1. `"Harry, Harry!"` — repeats on one line.
2. `Harry's wand` / `(Harry)` / `Harry—Ron` — punctuation adjacency must still match.
3. `Harrying`, `Harrys`, `Charry` — must **not** match (word-boundary check).
4. `Harry Potter` — one mention, not three (F8 longest-match).
5. `HARRY` in dialogue shouting — case-insensitive match.
6. Alias that is a prefix of another alias: `Ron` vs `Ronald`.
7. Alias spanning a line break: `"Harry\nPotter"` — decide and state the policy (recommended: matching does not cross line boundaries; call it out as a known limitation).
8. Character registered **after** the book is indexed.
9. Empty book / character with zero mentions → must return 0, not throw.
10. `insertLine` shifting every downstream line number — how do stored occurrences stay correct?
11. Unicode / accented names (`Fleur Delacour`) — normalization.

---

## 8. Evaluation rubric

**Strong hire**
- Domain model separated from indexing; index is a strategy/plug-in, not baked into `Book`.
- Ingestion is a single tokenizing pass; per-position lookup is a hash map or a trie/Aho-Corasick automaton for multi-word aliases. Explains *why* naïve per-alias scanning is O(text × aliases) and rejects it.
- Precomputed counts → O(1) `mentionCount`; occurrence lists appended in reading order, so `occurrences` needs no sort.
- Handles longest-match and word boundaries correctly, with tests.
- For F10, re-indexes only the affected line and uses a stable line identity (or a per-line occurrence bucket) so a shifted line number doesn't invalidate the index.
- Names the trade-offs: memory of full occurrence lists vs. counts only; trie build cost vs. query speed; immutability vs. incremental update.

**Hire**
- Correct working core with a `Map<CharacterId, Integer>` count and a `Map<CharacterId, List<Occurrence>>`.
- Tokenizes once per line and looks up tokens in an alias map (single-word aliases only), acknowledging multi-word as an extension.
- Reasonable class boundaries; handles case and boundaries; can state complexities when asked.

**No hire signals**
- `mentionCount` scans the book on every call, and the candidate doesn't notice when asked about a 10⁶-line book.
- `String.contains` / substring search per alias per line, with no word-boundary handling and no awareness of the `Harrying` false positive.
- Counting logic embedded inside `Book` or a single 200-line god class; no seam for a second index type.
- Cannot state the complexity of their own ingest loop.
- Never asks a single clarifying question and codes against an invented spec.

---

## 9. Reference design sketch (interviewer's notes — do not show the candidate)

**Core data structures, per book:**

```
countByCharacter        : Map<CharacterId, int>
countByCharacterChapter : Map<CharacterId, Map<ChapterId, int>>
occurrencesByCharacter  : Map<CharacterId, List<Occurrence>>      // append-only, reading order
charactersByLine        : Map<LineId, Set<CharacterId>>           // powers F9 co-occurrence
aliasTrie               : Aho-Corasick automaton over normalized aliases -> CharacterId
```

**Ingest (`addBook`)** — for each chapter, for each line:
normalize (lowercase, Unicode NFKC) → run the line through the automaton → keep only matches whose boundaries are non-word characters → resolve overlaps by preferring the longest match, then leftmost → append an `Occurrence`, bump both counters, and add to `charactersByLine`.

Cost: **O(total characters + number of matches)**, independent of the alias count. The single-word-only variant (tokenize the line, hash-lookup each token in `Map<String, CharacterId>`) is the acceptable simpler answer and is genuinely O(total characters) too — the automaton is what buys multi-word aliases.

**`topCharacters(k)`** — a size-k min-heap over `countByCharacter`: O(C log k), with C < 1000. A maintained sorted structure is over-engineering here; a candidate who justifies either is fine.

**Mutation (F10)** — the key insight is *line identity*. Give each line a stable `LineId` at insert time and keep `lineNumber` as a separate, derivable field; then `insertLine` doesn't invalidate stored occurrences. Bucket occurrences per line (`Map<LineId, List<Occurrence>>`) so `editLine` = subtract the old line's contribution from the counters, re-scan the new text, add the new contribution. O(length of the edited line).

**Concurrency (NF5)** — a `ReadWriteLock` per book, or copy-on-write of the per-line bucket plus atomic counters. The point is that the candidate scopes locking to a book, not to the whole library.

**Scaling beyond one machine** (only if there's time) — shard by `bookId`; the index is per-book, so it partitions cleanly. Counts are a natural fit for a precomputed materialized view; occurrence lists are the expensive part and can be recomputed on demand from stored text.

---

## 10. Follow-up questions to stretch a strong candidate

1. Character names are *not* given — infer them from the text. What changes? (Capitalization heuristics, frequency, an NER pass — and now the index must be rebuildable.)
2. Support "how many times do Harry and Draco appear within 3 lines of each other?" — a proximity query over the occurrence lists (merge two sorted lists, two-pointer).
3. A character is renamed mid-series (`Tom Riddle` → `Voldemort`). Merge two character indexes without a re-scan.
4. The library has 10⁵ books and you must answer `topCharacters` across all of them. What do you precompute?
5. Readers annotate books with their own private character lists. How do you avoid rebuilding a shared index per user?
