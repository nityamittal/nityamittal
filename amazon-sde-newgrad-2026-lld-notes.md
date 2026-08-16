# Amazon SDE New Grad Interview 2026 — LLD Questions & Community Notes

*Compiled 2026-08-16, covering roughly mid-June through mid-August 2026.*

## ⚠️ Sourcing note — read this first

The original ask was to scrape **r/amazonsdeprep** and **r/amazonemployees** on
Reddit directly. This environment's network egress proxy hard-blocks
`reddit.com` / `old.reddit.com` at the policy level (not a rate limit — a
403 on the CONNECT itself), and it also blocks direct fetches of
`teamblind.com`, `glassdoor.com`, and `leetcode.com`. So none of the content
below comes from opening those subreddits, and none of it is a verbatim
Reddit quote.

Instead, per your direction, this is compiled from **search-engine snippets
surfacing Blind (teamblind.com), Glassdoor, LeetCode Discuss, and Medium**
posts about Amazon new-grad SDE interviews. Two consequences worth flagging:
- I could **not** find anything specifically attributable to
  r/amazonsdeprep or r/amazonemployees — those two communities never
  surfaced in results.
- I could **not** find genuine first-person "testimonials" (offer stories,
  gratitude posts) with real dates/specifics from the last two months. What
  exists is general referral/offer chatter on Blind, not the kind of
  testimonial content you asked for. I'm not fabricating quotes to fill that
  gap — flagging it as a gap instead.

If you need the actual Reddit threads, the cleanest path is pulling them
yourself (logged into Reddit, or via a machine without this proxy policy)
and handing me the text/links to organize.

## Amazon new-grad SDE interview structure (2026, as reported)

Recent loops described in the search results:
- **~3 hours total**: 1 Leadership Principles (LP) round, 1 LLD/OOD round,
  1 DSA (coding) round — the most commonly cited structure.
- A June 2026 report described 4 rounds instead: behavioral, LLD, DSA, and
  a "hybrid AI behavioral + DSA" round (likely reflecting Amazon's newer
  Gen-AI-fluency interview component some 2025/2026 loops have added).
- Leadership Principles are woven into *every* round, not siloed to one —
  e.g. the LLD interviewer probing "Invent and Simplify," the coding
  interviewer probing "Deliver Results."
- Bar Raiser round is typically assessing: Customer Obsession, Ownership,
  Dive Deep, Deliver Results, Learn and Be Curious, Earn Trust.

## LLD/OOD questions reported (Amazon SDE, new-grad and nearby levels)

| Question | Level noted | Detail from source |
|---|---|---|
| Design a Parking Lot | New grad / SDE1 | Recurring "yes, expect OOP design" answer for new grads |
| Design a Restaurant | New grad | Paired with 2–3 LP questions in round 1 |
| Design a Chess Game | General SDE | Framed as "set up Java classes/interfaces," edge cases |
| Design a File Search System | New grad, onsite | Paired with 20-min LP + separate DSA/LP rounds |
| Design a File-System Storage Cache | New grad, **July 2026** | Candidate reported the round drifted into HLD territory more than LLD |
| Design a Tic-Tac-Toe API | General SDE | Framed explicitly as an LLD problem |
| Design a Vending Machine Leasing System | **SDE2** (not new grad) | State pattern for machine states (Idle, Out of Stock, Maintenance); VendingMachine/LeaseAgreement/PaymentStrategy relationships — included for pattern reference even though it's not new-grad level |
| Pizza order calculator | General SDE | Named alongside parking lot as a "classic" |

Consistent theme across sources: interviewers are grading on **class/interface
design, SOLID principles, extensibility to new requirements mid-interview,
and clearly narrated trade-offs** — not just a working solution.

## Testimonials / offer stories

Nothing substantive and dated within the last two months turned up. What
the searches did surface was generic and not source-specific enough to
quote responsibly:
- Referral-seeking posts from new grads targeting 2026 start dates.
- Offer-evaluation threads (compensation, location, team) rather than
  interview-process testimonials.
- One 2026 timeline (not clearly within the last two months) showing OA →
  virtual onsite → recruiter call spanning roughly a month.

None of this is from r/amazonsdeprep or r/amazonemployees specifically —
flagging again so it isn't mistaken for what was asked for.

## Sources (search-engine indexed, not directly fetched)

- [Amazon SDE 1 interview LLD, which one they'll ask?](https://www.teamblind.com/post/amazon-sde-1-interview-lld-which-one-theyll-ask-zuvrdgb4)
- [Amazon SDE 2 LLD interview questions](https://www.teamblind.com/post/Amazon-SDE-2-LLD-interview-questions-boK3UhLo)
- [Amazon New Grad Software Engineer Interview Questions — Glassdoor](https://www.glassdoor.com/Interview/Amazon-New-Grad-Software-Engineer-Interview-Questions-EI_IE6036.0,6_KO7,33.htm)
- [Amazon SDE New Grad Interview Experience & Questions — Glassdoor](https://www.glassdoor.com/Interview/Amazon-SDE-New-Grad-Interview-Questions-EI_IE6036.0,6_KO7,19.htm)
- [Amazon HLD LLD DSA Questions — LeetCode Discuss](https://leetcode.com/discuss/post/6906753/amazon-hld-lld-dsa-questions-by-anonymou-sepk/)
- [Amazon SDE II Interview Experience 2026 — LeetCode Discuss](https://leetcode.com/discuss/post/8457154/amazon-sde-ii-interview-experience-2026-19lzv/)
- [Step-by-Step Guide for FAANG Interviews in 2026 (Amazon LPs/Bar Raiser) — Medium](https://medium.com/@emilyhustlenyc/how-to-pass-the-amazon-software-engineer-interview-in-2026-leadership-principles-the-bar-raiser-c03901244baa)
- [Amazon Software Engineer Interview Guide for New Grads 2027 — Simplify.jobs](https://simplify.jobs/blog/amazon-swe-interview-new-grads)

## Recommended next step

If Reddit access matters for the final answer (it was the explicit ask),
the highest-value fix is reading r/amazonsdeprep and r/amazonemployees
directly yourself and pasting the threads here — I can then extract and
organize the actual LLD questions and testimonials from real content
instead of secondhand search summaries.
