# AGENTS.md
 
- You will never look under .old for any reason whatsoever.
- You will never write tests in my directory tree unless asked.

## Audience

Assume the user is a senior systems, embedded, and signal-processing engineer.

* Do not provide beginner tutorials.
* Do not explain standard language features unless they are directly relevant.
* Prefer precise technical reasoning over broad background material.

## Core Priorities

In descending order:

1. Correctness
2. Simplicity
3. Determinism
4. Maintainability
5. Performance
6. Brevity

Do not trade correctness or clarity for cleverness.

## Engineering Judgment

Before implementing the literal request, assess whether it indicates a deeper design problem.

* Identify incorrect assumptions, broken invariants, or architectural issues early.
* Challenge the requested approach when a simpler or more correct design exists.
* Distinguish between:
  * fixing the immediate symptom;
  * fixing the underlying design;
  * performing an unnecessary redesign.
* Do not expand scope without a concrete technical reason.
* When best practice is genuinely context-dependent, say so explicitly and explain the deciding factors.

Lead with the recommended approach. Mention alternatives only when they represent meaningful tradeoffs.

## Response Style

* Be concise and direct.
* Prefer code and concrete changes over lengthy explanation.
* Do not repeat the request.
* Avoid tutorials, motivational language, and generic best-practice lists.
* Explain non-obvious invariants, tradeoffs, and failure modes.
* State assumptions that materially affect correctness.
* Do not claim code is correct, safe, deterministic, or real-time suitable without checking the relevant conditions.

## Code Changes

* Make the smallest coherent change that solves the underlying problem.
* Preserve existing APIs and behavior unless changing them is necessary.
* Avoid speculative extensibility.
* Avoid unrelated cleanup.
* Do not introduce a new abstraction unless it:

  * enforces an invariant;
  * removes meaningful duplication;
  * clarifies ownership or state transitions;
  * materially improves testability.
* Prefer local, explicit code over indirection.
* Inline one-off logic when extracting it would only create a trivial helper.
* Extract code when doing so gives a concept a useful name, centralizes an invariant, or prevents duplication.
* Follow the existing codebase's naming, formatting, and error-handling conventions unless they are part of the problem.
* Update or add focused tests when behavior changes.
* Consider compile warnings, sanitizers, static analysis, and failure paths where relevant.

## C and C++

### Language

* Use C++17 for C++ code unless the project requires another standard.
* Respect the project's configured C standard for C code.
* Prefer standard-library facilities over custom equivalents when they meet the constraints.
* Do not suggest frameworks unless explicitly asked.

### Design

Favor Rust-like properties where practical:

* explicit ownership;
* value semantics;
* RAII;
* clear borrowing and lifetime relationships;
* explicit error propagation;
* exhaustive state handling;
* invalid states made difficult to represent;
* minimal hidden control flow;
* minimal hidden allocation.

Additional preferences:

* Prefer composition over inheritance.
* Avoid unnecessary virtual dispatch.
* Avoid pervasive getters, setters, factories, and interface hierarchies.
* Avoid `shared_ptr` unless ownership is genuinely shared.
* Prefer `unique_ptr`, values, references, spans, and views according to ownership.
* Prefer scoped enums and explicit state machines.
* Prefer immutable state and `const` where it improves reasoning.
* Avoid exceptions in real-time or embedded paths unless the existing project explicitly relies on them.
* Avoid macros when language features provide an equally clear solution.
* Do not use templates or concepts merely to avoid writing straightforward code.

### Performance

* Avoid dynamic allocation in hot paths.
* Prefer allocating during initialization and reusing fixed-capacity storage.
* Avoid unbounded work, blocking operations, locks, I/O, and hidden allocation in real-time paths.
* Consider cache locality, memory layout, branch behavior, contention, and worst-case execution time.
* Prefer data-oriented layouts where they improve access patterns.
* Do not optimize without identifying the relevant hot path or cost.
* Distinguish throughput, latency, average-case performance, and worst-case determinism.
* Treat benchmark results as hardware-, compiler-, and workload-dependent.

### Safety and Correctness

Check for:

* ownership and lifetime errors;
* iterator, pointer, and reference invalidation;
* integer overflow and narrowing;
* signed/unsigned mistakes;
* alignment, aliasing, and object-lifetime rules;
* undefined and implementation-defined behavior;
* concurrency and memory-ordering issues;
* partial initialization and cleanup failures;
* bounds violations;
* real-time violations caused by allocation, locking, logging, or system calls.

## Embedded and Real-Time Constraints

Assume Linux, embedded, and real-time constraints may apply.

* Performance and determinism matter.
* Prefer bounded memory and bounded execution.
* Make initialization-time work distinct from runtime work.
* Avoid assuming desktop-scale memory, threads, or operating-system services.
* Avoid uncontrolled background work.
* Treat logging, formatting, allocation, synchronization, and error reporting as potential real-time hazards.
* Clearly identify operations that are not safe for a hard real-time path.
* Do not describe ordinary Linux scheduling as hard real-time without supporting constraints.

## Design Preferences

Favor designs that are:

* simple;
* explicit;
* data-oriented;
* locally understandable;
* easy to debug;
* easy to test;
* resistant to invalid states.

Avoid:

* unnecessary abstractions;
* deep inheritance;
* dependency injection frameworks;
* service locators;
* excessive indirection;
* speculative generalization;
* tiny helpers that obscure simple control flow;
* design-pattern terminology used in place of reasoning;
* abstractions that hide ownership, allocation, synchronization, or control flow.

## Reference Standard

Prefer patterns demonstrated by mature, actively maintained systems code.

Judge a pattern by its technical properties rather than its source or popularity:

* clear invariants;
* explicit ownership;
* predictable control flow;
* limited dependencies;
* documented tradeoffs;
* strong testing;
* operational maturity.

Do not imitate large-company architecture when the scale or constraints do not justify it.

## Recommendations

When proposing a solution:

1. Give the preferred design.
2. Explain the key invariant or tradeoff.
3. Provide the minimal implementation.
4. Call out important risks or unresolved assumptions.
5. Mention alternatives only when they are materially different.

When reviewing code, prioritize:

1. correctness bugs;
2. undefined behavior;
3. ownership and lifetime problems;
4. concurrency and real-time hazards;
5. architectural problems;
6. unnecessary complexity;
7. style.