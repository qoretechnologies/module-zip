Minizip memory and comment ownership audit
=========================================

Copyright 2026 Qore Technologies, s.r.o.

Scope: the private minizip patch, CMake patch application, native regression
probe and Python driver. No Qore API or DataProvider changes.

The GCC truncation warning came from copying strlen bytes with strncpy into a
zero-filled comment allocation. Reviewing this path also found free-before-
validation and aliased-input use-after-free. Replacement now validates and
allocates first, copies the terminator, and then releases the previous comment.
The ownership ordering follows upstream develop commit
72d3b398c420f01f1351b1fc9ddb8fc53eb7faf4. The local patch also uses size_t before
the ZIP format's 65535-byte limit check.

The memory-stream warning came from copying a signed old capacity into a new
allocation without checking capacities. Reopening after reducing grow_size can
actually copy beyond the new allocation. The patch limits that copy, checks
negative inputs and growth before narrowing, and checks relative seek offsets
before addition. Allocation failure leaves the old allocation intact.

Five public-API regression groups pass on both the submodule (4.2.1) and the RPM
component (4.2.2), normally and with all Valgrind leak kinds treated as errors.
The probe and private library compile with -Wall -Wextra -Werror. Tests include
empty and maximum-length comments, oversize rejection, aliased comments,
injected allocation failures, shrinking reopen, normal growth, fixed buffers,
negative sizes and extreme positive/negative seek offsets. Allocation injection
is confined to the single-threaded test executable via the GNU linker.
The injection flag is volatile because GNU --wrap rewrites malloc references
after LTO; without volatile, GCC can reuse its pre-call value when checking
that the wrapper consumed the flag. The same five groups also pass under the
full Fedora RPM compiler/linker flags, including LTO, with Valgrind.

The full module's Debug build, API documentation, 61 Qore test cases (493
assertions), 90 feature assertions and ZIP CLI interoperability checks also pass.
No compiler or documentation warning is suppressed.

Run from the module repository on Linux with CMake, a C compiler and patch::

    python3 -B -W error -m unittest discover -s test -p test_minizip_memory.py -v
    QORE_TEST_VALGRIND=1 python3 -B -W error -m unittest discover -s test -p test_minizip_memory.py -v

Use QORE_MINIZIP_SOURCE_DIR to test an unpacked offline component. Tests copy and
patch this source under a private temporary directory and never modify it.

.. list-table:: Complete review checklist
   :header-rows: 1
   :widths: 48 8 44

   * - Check
     - Status
     - Evidence

   * - Entry exists in `doxygen/lang/120_modules.dox.tmpl` (for modules in the Qore repo; N/A for external module repos)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Entry exists in `doxygen/lang/900_release_notes.dox.tmpl` (for modules in the Qore repo; external modules have release notes in their .qm)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `qore_user_module()` or `qore_external_user_module()` call in `CMakeLists.txt`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Module added to QMOD list in `CMakeLists.txt`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `.qm` file has `@section <lowercasemodname>intro` as first doc section — must be all lowercase (e.g., `avrodataproviderintro`, not `AvroDataProviderintro`)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `%modern` in `.qm` file — no redundant `%new-style`, `%require-types`, `%strict-args`, `%enable-all-warnings`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - No parse directives (`%requires`, `%modern`, `%new-style`) in separated `.qc` files (check OUTSIDE of `@code` blocks only)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - No `%include` usage (deprecated for modules)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Copyright 2026 on all new files
     - Pass
     - Patch, probe, Python driver and this audit carry 2026 notices.

   * - Directory layout: `.qm` inside `qlib/<ModuleName>/` directory (not at `qlib/<ModuleName>.qm` for multi-file modules)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - No second `.qm` for the same module at `qlib/<ModuleName>.qm`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `ns=Qore::XX` matches the QoreNamespace constructor path
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `%modern` directive present
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Executable permission set (`chmod +x`)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Uses %prepend-module-path  before %requires for in-repo modules (Qore and Qore modules only; not Qorus)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - External module dependencies use `%try-module` — except modules delivered with the project itself (Qore ex: DataProvider, ConnectionProvider, QUnit, etc.) which use hard `%requires`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - No filesystem operations (fopen, open, creat, unlink, remove, rename, mkdir, rmdir, stat, chmod) without sandbox checks
     - N/A
     - Production changes operate on existing in-memory buffers; test fixtures use private temporary directories.

   * - No network operations (connect, bind, socket, getaddrinfo, gethostbyname) without sandbox checks
     - N/A
     - Production changes operate on existing in-memory buffers; test fixtures use private temporary directories.

   * - If filesystem/network ops exist, verify `QoreSandboxManagerHelper` usage
     - N/A
     - Production changes operate on existing in-memory buffers; test fixtures use private temporary directories.

   * - No `File::`, `Dir::`, `Socket::`, `HTTPClient::` usage without justification
     - N/A
     - Production changes operate on existing in-memory buffers; test fixtures use private temporary directories.

   * - All `for`/`while` loops that could iterate >100 times have `qore_check_cancel()` checks
     - N/A
     - No new production loops or blocking operations; test seek loop has three iterations.

   * - Uses `qore_check_cancel()` (NOT deprecated `qore_check_io_interrupt()`)
     - N/A
     - No new production loops or blocking operations; test seek loop has three iterations.

   * - Check frequency: every 100 iterations for tight loops, every 10 for expensive iterations
     - N/A
     - No new production loops or blocking operations; test seek loop has three iterations.

   * - No blocking operations without cancellation support
     - N/A
     - No new production loops or blocking operations; test seek loop has three iterations.

   * - Every action has `display_name`, `short_desc` (plain text, <80 chars), `desc` (markdown)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Every action has `options` populated via `getActionOptionFromFields()` — without this, the action shows an empty, unusable form
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Every action has `output_type` set to a typed data type constant (e.g., `MyResponseDataType`) — not omitted
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - DPAT_API actions: provider has `"supports_request": True` and implements `doRequestImpl()`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - DPAT_FIND actions: every option exists in `SearchOptions`, `getRecordTypeImpl()` returns `*hash<string, AbstractDataField>`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Scheme-based apps (with `"scheme"` in registerApp): actions use `"path"` and do NOT use `"cls"` — having both `scheme` and `cls` causes a runtime error
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Single-key hash slices use trailing comma: `Fields{"key",}` (without trailing comma, `Fields{"key"}` returns the value, not a hash)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Typed data type classes exist for request and response types — inherit `HashDataType`, have `const Fields` hash, call `addQoreFields(Fields)` in constructor, export public constant at bottom (e.g., `public const MyDataType = new MyDataType();`)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Request/input types use `public` Fields (enables `ClassName::Fields` in action registration)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Response/output types use `private` Fields
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Each field in data types has `display_name`, `type`, and `desc` (markdown-formatted)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Input fields have `example_value` where useful (string fields, endpoint URIs, SQL queries, etc.)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Fields with finite allowed values use `allowed_values` with `AllowedValueInfo` containing both `value` and `display_name` (Title Case, human-readable) — never bare values, never described only in text
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Password/secret fields have `"sensitive": True`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `groups` uses `AppGroup` enum values from `qlib/DataProvider/AppGroup.qc`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - App `logo` stored as separate file, loaded at module level in `Priv` namespace
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - App `desc` uses markdown: bullet list of capabilities, links to project website, business-language explanation of value
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `display_name` is user-friendly ("Apache Avro" not "avro")
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `short_desc` is plain text, under 80 chars, single sentence — no markdown
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `desc` uses markdown: backticks for code/field refs (`` `field_name` ``, `` `True` ``, `` `pdf` ``), `\n\n` for paragraphs, `- ` bullet lists for enumerations, `bold` for caveats
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Descriptions use plain business language relating to common challenges — not just technical "what" but "why" and "when to use"
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - No bare `True`/`False`/`NOTHING` — must be backtick-wrapped in `desc`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - No bare field/option names in prose — must use backticks
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Long descriptions (>500 chars) use bold section headers and bullet lists
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Factory registration in Qore repo: every factory name registered in `qlib/DataProvider/DataProvider.qc` → `FactoryMap` (without this, module loads but doesn't appear in Qorus apps)
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - `getRecordTypeImpl()` signature: must be `private *hash<string, AbstractDataField> getRecordTypeImpl(*hash<auto> search_options)` — NOT returning `*AbstractDataProviderType`
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Dependency JARs committed (for JNI modules): JAR files in `qlib/*/jar/` may be gitignored — use `git add -f` to ensure they're tracked, otherwise CI compilation fails
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - JAR install rules in CMakeLists.txt for all dependency JARs
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - No workarounds: No TODOs, FIXMEs, stubs, or partially-implemented features
     - Pass
     - Fixes ownership and arithmetic at their source; no diagnostic suppression or skipped failing cases.

   * - Exception safety: C++ uses `ReferenceHolder` for Qore allocations, `std::unique_ptr` for C++ allocations, `*xsink` checked after every fallible operation
     - Pass
     - Validation and allocation precede mutation; allocation-failure tests verify original data survives. Temporary-directory cleanup is registered before compilation.

   * - Thread safety: All mutable shared state protected by `std::lock_guard<std::mutex>` or documented as immutable-after-construction
     - Pass
     - Production state remains per stream/archive; no new global state. Failure injection is explicitly single-threaded and test-only.

   * - Type safety: Strongly-typed `code<return(args)>` instead of untyped `code`; `static_cast` instead of C casts; typed hashdecls for results; enums where appropriate
     - Pass
     - size_t holds string lengths, int64_t holds growth, narrowing follows bounds checks; C casts match the bundled C API.

   * - Performance: No O(n²) where O(n) is possible; no unnecessary copies; coordinate descent uses incremental residuals not full matrix multiply
     - Pass
     - Constant-time bounds checks and one bounded copy per existing resize/replacement; no added production loops.

   * - Error handling: All inputs validated (dimensions, empty data, unfitted models); C++ I/O handles EAGAIN/EINTR if applicable
     - Pass
     - Negative sizes, overflowing growth/seeks, invalid comments and allocation failures return existing minizip error codes without dangling ownership.

   * - Documentation: Doxygen `@param`, `@return`, `@throw` on all public methods; `@par Example` with realistic business scenarios; `@note` for important caveats
     - Pass
     - Release notes and this audit explain the behavior, provenance, test commands and platform scope.

   * - QPP flags: `[flags=CONSTANT]` on methods that never throw; `[flags=RET_VALUE_ONLY]` on methods that throw but have no side effects
     - N/A
     - No new Qore module, DataProvider, QPP interface or Qore-language test.

   * - Security: No user-controlled format strings; no buffer overflows; bounds checking on array indices; no credentials in code
     - Pass
     - Regression tests exercise the use-after-free, double-free and oversized-copy failure paths; no credentials or user-controlled format strings.

   * - Correctness: Algorithms verified against reference implementations; edge cases tested (empty data, single sample, all-zero features)
     - Pass
     - Five regression groups on both source versions, Valgrind, module integration and CLI interoperability all pass.
