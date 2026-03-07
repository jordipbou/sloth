Everything corrected except 2. File access implementation.

# Sloth C and Java Implementation Synchronization Report

This document compares the canonical C implementation (`platforms/c/sloth.h`) with the Java implementation (`platforms/java/src/main/java/io/github/jordipbou/sloth/Sloth.java`).

## 1. Differences in Java Implementation (Potential Issues)

### Missing `DUP` Primitive
The Java `bootstrap_kernel` is missing the `DUP` primitive. This is a fundamental Forth word. While `0 PICK` can substitute for `DUP`, many Forth programs and the bootstrap process itself expect `DUP` to be defined as a primitive for performance and standard compliance.

### Number Parsing Bug in `_interpret_`
In `Sloth.java`, the `_interpret_` method detects prefixes for different bases (`#` for decimal, `$` for hex, `%` for binary) and sets `temp_base` accordingly. However, it calls `Integer.parseInt(buf.toString())` without passing `temp_base`.
**Fix:** Change to `Integer.parseInt(buf.toString(), temp_base)`.

### Segmented Memory Model
Java uses an `ArrayList<ByteBuffer>` with a custom addressing scheme: `(block_index << 24) | byte_offset`. 
- **C:** Flat memory model using absolute pointers.
- **Risk:** Forth code that performs pointer arithmetic between different memory blocks (e.g., comparing a pointer in the dictionary to one in the user area) will fail in Java if it assumes a linear relationship.

### Character Size (`sCHAR`)
- **C:** `suCHAR` is 1 byte.
- **Java:** `sCHAR` is 2 bytes (matching Java's `char` / UTF-16).
- **Impact:** This is an intentional design choice for the Java port, but it means that standard Forth code assuming `1 CHARS` is 1 byte will need adjustment.

### Loop Implementation Divergence
Java implements `(DOLOOP)` and `UNLOOP` as primitives and uses specific user variables (`(IX)`, `(JX)`, `(KX)`, `(LX)`) to manage loop state. The C version does not have these primitives, suggesting it implements loops entirely in Forth or uses a different mechanism.

---

## 2. Implementing File Access in Java

The Java version currently has `// TODO` markers for `REFILL`, `INCLUDED`, and file-related parts of `RESTORE-INPUT`.

### Proposed Strategy:
1.  **File Management:** Add a `private Map<Integer, RandomAccessFile> files = new HashMap<>();` and an `int nextFileId = 1;` to the `Sloth` class.
2.  **`INCLUDED`:**
    *   Iterate through search paths (stored at `PATHS` in the user area).
    *   Open the file using `RandomAccessFile`.
    *   Save current `SOURCE_ID`, `IBUF`, `IPOS`, `ILEN`.
    *   Read the file line by line, setting `IBUF` and calling `INTERPRET` for each.
    *   Restore previous input state upon completion or error.
3.  **`REFILL`:**
    *   Get the `RandomAccessFile` associated with the current `SOURCE_ID`.
    *   Read a line using `readLine()`.
    *   Store the line in a dedicated `ByteBuffer` (to avoid the C version's "miracle" local buffer issue).
    *   Update `ILEN` and reset `IPOS` to 0.

---

## 3. Unsynchronized Primitives

The following primitives are present in one implementation but missing from the other's `bootstrap_kernel`:

| Primitive | In C? | In Java? | Notes |
| :--- | :---: | :---: | :--- |
| `DUP` | Yes | **No** | Critical synchronization issue. |
| `PICK` | No | Yes | Java has it, C doesn't (C uses it internally). |
| `RPICK` | No | Yes | |
| `DEPTH` | No | Yes | |
| `RDEPTH` | No | Yes | |
| `M*` | No | Yes | |
| `D+` | No | Yes | |
| `(DOLOOP)` | No | Yes | |
| `UNLOOP` | No | Yes | |
| `B@`, `B!` | No | Yes | Byte access (Java segmented memory helper). |
| `W@`, `W!` | No | Yes | 16-bit access. |
| `L@`, `L!` | No | Yes | 32-bit access. |
| `X@`, `X!` | No | Yes | 64-bit access. |
| `(CBUF)` | 64 | 64 | Constant is synchronized. |

### User Area Variables Comparison
Java includes several variables not found in C's `sloth.h`:
- `(IX)`, `(JX)`, `(KX)`, `(LX)`: Used for the primitive loop implementation.
- `(INTERPRET)` is at `27*sCELL` in C but `31*sCELL` in Java.
- `PATHS` displacement differs: `31*sCELL` in C vs `35*sCELL` in Java.

**Recommendation:** Realign user area displacements between C and Java to ensure that any Forth code accessing these by hardcoded offsets remains compatible.
