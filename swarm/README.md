# ZEN Swarm System

This directory contains a component-based system for generating Claude Code sub-agents that collaborate to implement the ZEN language interpreter.

## Architecture

```
swarm/
├── components/          # Reusable DNA components
├── specializations/     # Worker specialization data
├── dna/                # Assembled DNA files (generated)
└── assemble-dna.js     # Assembles components into DNA
```

## Overview

The swarm consists of three agent types:
- **Queen**: Strategic coordinator (1 instance)
- **Architect**: System designer (1 instance) 
- **Workers**: Component implementers (unlimited instances)

## Quick Start

```bash
# Initialize the swarm with basic agents
../swarm.sh init

# Create a named swarm with all workers
../swarm.sh create-swarm swarm-1

# Add specialized workers to default swarm
../swarm.sh add-worker runtime
../swarm.sh add-worker types
../swarm.sh add-worker memory

# Check swarm status
../swarm.sh status
```

## Using the Swarm

After generating agents, use them in Claude Code:

1. Run `/agents` to see available agents
2. Use explicit invocation:
   ```
   "Use the zen-queen agent to analyze current progress and suggest next steps"
   "Use the zen-architect agent to design the value type system"
   "Use the zen-worker-lexer agent to implement number token parsing"
   ```

## Agent Roles

### Queen Agent
- Analyzes project state with `make vision`
- Identifies critical implementation paths
- Recommends task assignments
- Does NOT implement code

### Architect Agent  
- Designs component interfaces
- Chooses algorithms and data structures
- Creates implementation specifications
- Does NOT implement code

### Worker Agents
- Implement based on specifications
- Follow MANIFEST.json exactly
- Ensure code quality and testing
- Can specialize in specific areas

## Component System

### Components
The `components/` directory contains reusable DNA building blocks:
- `base-header.md`: Agent identification template
- `prime-directive.md`: Core mission statement
- `sub-agent-context.md`: Claude Code integration
- `project-context.md`: ZEN project information
- `workflow-start.md`: Initial commands to run
- `coding-standards.md`: Code style guidelines
- Role-specific components for capabilities and output formats

### Specializations
The `specializations/` directory contains YAML files defining worker expertise:
- `lexer.yaml`: Tokenization patterns and techniques
- `parser.yaml`: AST construction and grammar
- `types.yaml`: Value system implementation
- `runtime.yaml`: Execution engine patterns
- `memory.yaml`: Memory management strategies
- `stdlib.yaml`: Built-in function patterns

### DNA Assembly
The `assemble-dna.js` script combines components with specialization data:
1. Loads component templates
2. Substitutes variables ({{AGENT_TYPE}}, {{SPECIALIZATION}}, etc.)
3. Generates role-specific DNA files
4. Saves to `dna/` directory

### Generated DNA
The assembled DNA files are created dynamically:
- `dna/queen.md`: Strategic coordinator DNA
- `dna/architect.md`: System designer DNA  
- `dna/worker-*.md`: Specialized worker DNA

## Workflow Example

1. **Strategic Planning**
   ```
   "Use the zen-queen agent to analyze what needs to be done next"
   ```

2. **Design Phase**
   ```
   "Use the zen-architect agent to design the parser for arithmetic expressions"
   ```

3. **Implementation**
   ```
   "Use the zen-worker-parser agent to implement the arithmetic parser design"
   ```

## Key Principles

- **Separation of Concerns**: Each agent type has a specific role
- **No Overlapping**: Agents don't step on each other's responsibilities
- **Manifest Compliance**: All work follows MANIFEST.json specifications
- **Quality First**: Every implementation includes testing and documentation

## Extending the System

### Adding New Worker Specializations
1. Create a YAML file in `specializations/`:
   ```yaml
   name: optimizer
   description: "performance optimization"
   details: |
     - Profile and identify bottlenecks
     - Optimize algorithms and data structures
   focus_areas: |
     - Cache optimization
     - Algorithm complexity reduction
   key_patterns: |
     ```c
     // Example patterns
     ```
   ```

2. Run DNA assembly:
   ```bash
   node swarm/assemble-dna.js
   ```

3. Generate the agent:
   ```bash
   ../swarm.sh add-worker optimizer
   ```

### Modifying Components
Edit files in `components/` to change behavior across all agents:
- Update `coding-standards.md` for new style rules
- Modify `workflow-start.md` to change initial commands
- Enhance output formats in role-specific components

After changes, reassemble DNA and regenerate agents.

## Generated Agents

Agents are created in `.claude/agents/` as markdown files with YAML frontmatter:

```markdown
---
name: zen-queen
description: Strategic coordinator for ZEN language implementation - analyzes project state and recommends task assignments
---

# Queen AGENT DNA
...
```

Generated files:
- `zen-queen.md` - Strategic coordinator
- `zen-architect.md` - System designer
- `zen-worker-lexer.md` - Lexer specialist
- `zen-worker-parser.md` - Parser specialist
- `zen-worker-runtime.md` - Runtime specialist
- `zen-worker-types.md` - Type system specialist
- `zen-worker-memory.md` - Memory management specialist
- `zen-worker-stdlib.md` - Standard library specialist