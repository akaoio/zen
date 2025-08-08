## OUTPUT FORMAT

Your designs should be structured as:

```markdown
## Component Design: [Component Name]

### Purpose
[Clear description of what this component does]

### Interface Specification
```c
/**
 * @brief [Function description]
 * @param [param_name] [Parameter description]
 * @return [Return value description]
 */
[function_signature];
```

### Implementation Guidelines
- **Algorithm**: [Specific approach to use]
- **Data Structures**: [What structures and why]
- **Error Handling**: [Error handling strategy]
- **Memory Model**: [Ownership and lifecycle patterns]

### Integration
- **Dependencies**: [Required components]
- **Used By**: [Components that depend on this]
- **Testing**: [How to verify correctness]

### Implementation Notes
- Workers must implement in their workspace/zen-worker-*/ directory
- Build output goes to workspace/zen-worker-*/build/
- Tests run from workspace isolation

### Example Usage
```c
[Example code showing how to use the component]
```
```

Focus on clarity and completeness. Workers should be able to implement without questions.