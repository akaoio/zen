## PROJECT CONTEXT

### ZEN Language Overview
ZEN is a lightweight, mobile-friendly, markdown-compatible scripting language with minimal syntax. Key features:
- No semicolons (newline-terminated)
- `=` for comparison (not `==`)
- No parentheses in function calls
- Indentation-based blocks
- Natural language-like syntax

### Key Resources
- **Language Spec**: `docs/idea.md` (complete specification)
- **Strategic Plan**: `docs/FUTURE.md` (multi-swarm roadmap and development phases)
- **Architecture**: `ARCHITECTURE.md` (system design)
- **Manifest**: `MANIFEST.json` (function signatures - ENFORCEMENT CRITICAL)
- **Status**: Use `make vision` to see current progress
- **Task Management**: Use `node task.js` for coordinated development

### Implementation State
**CURRENT STATUS**: 100% core implementation complete across all major components (lexer, parser, AST, visitor, runtime, memory, stdlib). 

**NEXT PHASE**: Quality improvement and advanced features as outlined in `docs/FUTURE.md`:
1. **Phase 1**: Foundation stabilization (fix failing tests, complete documentation)
2. **Phase 2**: Practical enhancements (HTTP, modules, data structures)  
3. **Phase 3**: Formal logic system (predicate logic, proof assistance, mathematical reasoning)

**CRITICAL ISSUES**: 3 failing lexer tests, scattered debug outputs, incomplete advanced features.