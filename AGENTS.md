# AGENTS

## Project Overview

Goal: create a new IR representation for statically allocated stack variables (function locals) that satisfies the constraints below, replacing the current `alloca` + `llvm.lifetime.start/end` model.

Background: llvm/llvm-project#45725 shows the current model is semantically self-inconsistent: InstSimplify folds `icmp` of distinct allocas assuming that distinct allocations have distinct addresses, while stack coloring may place allocas with disjoint lifetimes at the same address. This yields miscompilations in C and safe Rust (`icmp` folds and `ptrtoint` arithmetic become inconsistent); see also #51838, #54002, #131927, #104776. Lifetime intrinsics cannot be retrofitted with coherent semantics; several replacement designs have been discussed in the issue, but none is accepted. Current state: lifetimes are restricted to allocas and their size operand was removed (#149310, #150248), and `llvm.structured.alloca` (#186811) covers only opaque physical layouts. The issue remains open, with no accepted design yet.

Constraints:
- Lifetimes must be precisely computable and verifiable from the IR alone: no walking all uses of an allocation to find its lifetime markers; well-formedness is enforced by the verifier.
- Non-overlapping lifetimes may share storage; overlapping lifetimes must not; the program must behave as if overlapping allocations never overlap.
- All locals are allocated on entry to a static stack frame; inlining must not excessively bloat the stack.
- Address semantics must be consistent: distinct live allocations compare unequal; `icmp eq` folds to false only for overlapping lifetimes and does not fold for non-overlapping ones; `ptrtoint` results stay consistent; each lifetime has fresh provenance and pointers die with their lifetime.
- Optimizer integration must not regress, including alias analysis; storage reuse must be reliably computable rather than depending on best-effort stack coloring.
- Exception handling (landing pads) needs a defined interaction.
- Hard budget: no sacrificing optimization results, compile time, or compatibility (existing IR, frontends, and backends; existing IR needs an upgrade path).

## Project Constraints

LLVM cmake build is in ./build.

CMake configure command:
```bash
cmake ../llvm -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja -DLLVM_TARGETS_TO_BUILD="X86" -DBUILD_SHARED_LIBS=ON \
-DCMAKE_C_COMPILER=gcc-16 -DCMAKE_CXX_COMPILER=g++-16 -DLLVM_ENABLE_PROJECTS="clang;" -DLLVM_PARALLEL_LINK_JOBS=4 \
-DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ABI_BREAKING_CHECKS=WITH_ASSERTS -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF \
-DLLVM_ENABLE_WARNINGS=ON -DLLVM_APPEND_VC_REV=OFF -DLLVM_ENABLE_BINDINGS=OFF \
-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
-DCLANG_ENABLE_STATIC_ANALYZER=OFF -DCLANG_ENABLE_CIR=OFF
```
Don't re-configure yourself.

To build LLVM (run from `/llvm/build`):
```
cmake --build . -j 32 -t <target>
```
Always use incremental build. Never clean the build dir.

### Normative documents

The following documents (paths relative to the repository root) apply to local
development in this checkout. Consult the local copy in this tree, not the
published website:

- `llvm/docs/CodingStandards.md` — required language level, style, naming, and comment rules.
- `llvm/docs/TestingGuide.md` — test infrastructure and where tests must be added.
- `llvm/docs/InstCombineContributorGuide.md` — applies to all middle-end and back-end development workflows (precommit tests, `update_test_checks.py`, Alive2 proofs, canonicalization, target independence), but is advisory, not mandatory.

### Style and validation

- Format touched files with `clang-format` (repo-root `.clang-format`); it is the authority for style.
- For style rules that cannot be automated, best-effort compliance is enough; the user performs the final cleanup before upstreaming.
- Back-end changes only need the X86 tests to pass.

### Artifact locations

- Research documents that are worth landing go in the `research/` directory at the repository root.
- Temporary files that must be retained longer (downloads, crawled content, etc.) go in the `work/` directory at the repository root; `work/` is gitignored.
- One-off temporary files go in the default tmp directory (`/tmp/opencode`).

## Repository Language Rules

- Write all repository content in English and reply to the user in Chinese (中文).

## User Interaction Rules

- Explicit user instructions take precedence over everything in this file: if they conflict, follow the user, and the most recent instruction wins. User instructions may be temporary and override this file for as long as they apply.
- If requirements or plans are ambiguous, eliminate disagreement instead of guessing: first explore the codebase when that can answer the question, otherwise ask clarifying questions aggressively, preferably one at a time.
- Clarifying questions must drive toward shared understanding by walking the design tree and resolving decision dependencies; each question must cover purpose, constraints, success criteria, or scope boundaries as appropriate, include all viable options, avoid requiring free-form input, and state the recommended answer.
- After the user approves execution, keep going until the full planned task is complete. Do not stop for intermediate progress reports, return control while the approved goal is only partially implemented, or ask whether to continue with obvious next steps, natural follow-ups, or clear previews, summaries, or refinements unless a genuine blocker, real ambiguity, or material risk requires user input.
- If something should obviously be done now, do it instead of deferring it to "later", "next", or a "follow-up". When reviewing or refining in-progress work, immediately implement any concrete fix you understand and that is not blocked.
- Resolve encountered difficulties autonomously whenever possible.
- When handing control back to the user after task completion, end the response with `Done.`

## Design and Planning Rules

- Before proposing a design or implementation, inspect the current project context, including relevant files, documentation, and recent changes.
- State assumptions explicitly. If materially different interpretations remain after exploration, surface and resolve them instead of silently choosing one.
- For feature work, behavior changes, and other creative or architectural tasks, complete a design step before implementation. If the request is too broad for one coherent spec or plan, decompose it into smaller subprojects and handle them one at a time.
- After gathering enough context, present 2-3 viable approaches with trade-offs and a recommended option, size the design sections to the topic, and validate the design with the user before implementation.
- Before substantial implementation, define concrete success criteria and explicit checks that prove completion; prefer tests, builds, or other verifiable validation over vague goals.
- Unless the user explicitly asks for a temporary workaround, limited experiment, reduced-scope language design, or backwards compatibility, design and implement the final intended product: complete behavior, durable interfaces, maintainable structure, the full language design rather than a staged or minimal subset, and clean breaking changes instead of compatibility shims during rapid iteration.
- If a requested feature depends on a lower-level prerequisite, implement that prerequisite as part of the work instead of lowering the acceptance bar, trimming scope, or presenting a partial workaround as complete.
- Do not limit the solution to the smallest easy increment or patch size when the correct solution requires broader structural changes. Make decisive, coherent refactors, including renaming APIs, reshaping module boundaries, or replacing flawed internal structures when needed.
- Design systems as small, well-bounded units with clear responsibilities, explicit interfaces, and dependencies that are easy to understand and test independently.
- In existing codebases, follow established patterns unless changing them is necessary to support the current goal.
- Apply YAGNI strictly. Make focused changes, avoid unrelated reformatting or cleanup, and remove only unrequested features, speculative abstractions, unnecessary complexity, or artifacts made unused by your own change unless broader cleanup was requested.

## Git Commit Rules

- Work purely locally on the current branch (`lifetime-research`): do not push, do not open pull requests, and preserve a linear history.
- Do not amend, rewrite, replace, or reorder existing commits unless the user explicitly requests it.
- Before starting work, ensure the worktree is clean; if prior changes are present, review them and commit the relevant ones before beginning new task work.
- Commit changes automatically after each completed milestone without waiting for the user to ask. After finishing a logical unit of work that passes tests, immediately stage and commit with an LLVM-style subject, then ensure the worktree is clean.
- Never commit temporary files, scratch artifacts, ad hoc notes, or other non-durable byproducts.
- Use LLVM-style one-line commit subjects of the form `[Scope] Title` (e.g. `[InstCombine] Fold select of constants`). Do not include any commit message body.
- Split commits by logical unit so they stay focused and reviewable.
