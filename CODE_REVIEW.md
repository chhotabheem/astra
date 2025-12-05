# Code Review Checklist

> **Purpose**: Quality standards and checklist for code reviews.  
> **Last Updated**: 2025-12-05  
> **Version**: 1.0

---

## Pre-Submission Checklist

### Before Submitting Code

- [ ] **Mental Dry Run**: Reviewed code 5+ times before compilation
- [ ] **Compilation is for verification**, not debugging
- [ ] **All tests pass**: Run `ctest --test-dir build --output-on-failure`
- [ ] **No warnings**: Built with `-Wall -Wextra -Wpedantic`

---

## Code Quality Checklist

### Clean Architecture

- [ ] Dependencies point inward (dependency rule)
- [ ] Business logic isolated from infrastructure
- [ ] Uses interfaces, not concrete implementations

### Modern C++ (Effective Modern C++)

- [ ] RAII for all resources
- [ ] No raw pointers (use `std::unique_ptr`, `std::shared_ptr`)
- [ ] Move semantics used where applicable
- [ ] `[[nodiscard]]` on critical return values
- [ ] `noexcept` where appropriate

### Thread Safety

- [ ] No data races
- [ ] Proper synchronization (mutexes, atomics)
- [ ] Thread pool usage follows SEDA model

### Resilience (Release It!)

- [ ] Timeouts on all blocking operations
- [ ] Error handling at boundaries
- [ ] Graceful degradation on failures

### Testing

- [ ] Unit tests included
- [ ] Tests follow TDD principles
- [ ] Edge cases covered

---

## Sanitizer Verification

Before merging, code should pass:

| Sanitizer | Purpose |
|:----------|:--------|
| **ASan** | Memory safety (use-after-free, buffer overflow) |
| **TSan** | Thread safety (data races) |
| **Valgrind Memcheck** | Memory leaks |
| **Valgrind Helgrind** | Data races |

---

## Related Documents

- [DESIGN_PHILOSOPHY.md](file:///home/siddu/astra/DESIGN_PHILOSOPHY.md) - Why we follow these standards
- [ARCHITECTURE.md](file:///home/siddu/astra/ARCHITECTURE.md) - System architecture
- [README.md](file:///home/siddu/astra/README.md) - Build commands

---

**End of Document**

---

## Language Standards

| Standard | Status |
|:---------|:-------|
| **C++17** | ✅ Required |
| **C++20** | ❌ Not allowed |

**Why C++17 only**: Project standard. No C++20 concepts or features.
