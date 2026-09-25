# Date & Timestamp Literals

This page defines the syntax, precision, validation rules, and defaulting behavior for date and timestamp literals in MemoraDB.

---

## Supported Formats

MemoraDB supports five levels of granularity for date and timestamp literals:

| Format Pattern | Granularity | Example |
|---|---|---|
| `YYYY-MM-DD` | Date only | `2026-09-20` |
| `YYYY-MM-DD HH` | Hour | `2026-09-20 14` |
| `YYYY-MM-DD HH:MM` | Minute | `2026-09-20 14:30` |
| `YYYY-MM-DD HH:MM:SS` | Second | `2026-09-20 14:30:45` |
| `YYYY-MM-DD HH:MM:SS.mmm` | Millisecond | `2026-09-20 14:30:45.250` |

---

## Defaulting Rules

When partial time components are omitted, MemoraDB auto-fills missing fields with zeroes:

| Input Literal | Hour | Minute | Second | Millisecond |
|---|---|---|---|---|
| `2026-09-20` | `00` | `00` | `00` | `000` |
| `2026-09-20 14` | `14` | `00` | `00` | `000` |
| `2026-09-20 14:30` | `14` | `30` | `00` | `000` |
| `2026-09-20 14:30:45` | `14` | `30` | `45` | `000` |
| `2026-09-20 14:30:45.250` | `14` | `30` | `45` | `250` |

### Millisecond Formatting Rules
If a fraction is provided after seconds:
* It is normalized to 3 digits.
* `.5` becomes `.500` ms.
* `.05` becomes `.050` ms.
* Longer fractions are truncated to 3 digits (e.g., `.1234` becomes `.123`).

---

## Validation Ranges

The parser validates all date and time components:

* **Year**: Any 4-digit integer (`1900` to `9999`).
* **Month**: `1` through `12`.
* **Day**: `1` through `maxDay`, computed dynamically based on the month and leap year rules (`isLeapYear(year)`).
* **Hour**: `0` through `23`.
* **Minute**: `0` through `59`.
* **Second**: `0` through `59`.
* **Millisecond**: `0` through `999`.

If any component falls outside these ranges, the parser throws a descriptive `ParseError` indicating the line and column number.

---

## The Bare Date Rule in `AS OF` and `SNAPSHOT`

An intentional design decision in MemoraDB is how a **bare date** (`YYYY-MM-DD` with no time portion) behaves in point-in-time queries:

```sql
-- Evaluates state at the END of that calendar day (23:59:59.999)
SELECT * FROM notes AS OF 2026-09-20;

-- Evaluates state at the EXACT FIRST INSTANT of that calendar day (00:00:00.000)
SELECT * FROM notes AS OF 2026-09-20 00:00:00.000;
```

* **Rationale**: When a user queries "as of September 20", they typically expect to see data that was created during that day. Evaluating at `23:59:59.999` includes all activity from that date.
* If you specifically need the start of the day before any business activity occurred, specify the time explicitly as `00:00:00.000`.
