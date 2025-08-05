#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const yaml = require('js-yaml');

class DNAAssembler {
    constructor(swarmDir, swarmId = null) {
        this.swarmDir = swarmDir;
        this.componentsDir = path.join(swarmDir, 'components');
        this.specializationsDir = path.join(swarmDir, 'specializations');
        this.dnaDir = path.join(swarmDir, 'dna');
        this.swarmId = swarmId;
    }

    loadComponent(name) {
        const filePath = path.join(this.componentsDir, `${name}.md`);
        if (fs.existsSync(filePath)) {
            return fs.readFileSync(filePath, 'utf8');
        }
        return '';
    }

    loadSpecialization(name) {
        const filePath = path.join(this.specializationsDir, `${name}.yaml`);
        if (fs.existsSync(filePath)) {
            const content = fs.readFileSync(filePath, 'utf8');
            return yaml.load(content);
        }
        return {};
    }

    substituteVariables(content, variables) {
        let result = content;
        for (const [key, value] of Object.entries(variables)) {
            const placeholder = `{{${key}}}`;
            result = result.split(placeholder).join(String(value));
        }
        return result;
    }

    generateSwarmActivationExample(role) {
        if (!this.swarmId) return '';
        
        const examples = {
            queen: `<example>Context: User wants to coordinate work within ${this.swarmId}. user: "${this.swarmId} analyze progress" assistant: "I'll activate the ${this.swarmId} queen to analyze the current state and coordinate next steps" <commentary>The swarm ID ensures this queen coordinates only within ${this.swarmId} and doesn't interfere with other swarms.</commentary></example>`,
            architect: `<example>Context: User needs design work within ${this.swarmId}. user: "${this.swarmId} design parser module" assistant: "I'll use the ${this.swarmId} architect to create the technical specification" <commentary>The architect will design within the context of ${this.swarmId}'s specific requirements.</commentary></example>`,
            worker: `<example>Context: User needs implementation within ${this.swarmId}. user: "${this.swarmId} implement lexer functions" assistant: "I'll activate the ${this.swarmId} workers to implement the lexer components" <commentary>Workers in ${this.swarmId} will coordinate through their swarm's queen to avoid conflicts.</commentary></example>`
        };
        
        return examples[role] || '';
    }

    generateSwarmCommands() {
        if (this.swarmId) {
            return `- \`${this.swarmId} work\` - Activate all ${this.swarmId} agents for parallel work\n- \`${this.swarmId} status\` - Get status from all ${this.swarmId} agents\n- \`${this.swarmId} continue\` - Continue work with all ${this.swarmId} agents`;
        } else {
            return '- `swarm work` - Activate all agents for parallel work\n- `swarm status` - Get status from all agents\n- `team-1 work` - Activate team 1 agents (if you\'re part of team 1)';
        }
    }

    assembleQueenDNA() {
        const components = [
            'base-header',
            'prime-directive',
            'sub-agent-context',
            'project-context',
            'multi-swarm-awareness',  // Multi-swarm system awareness
            'queen-capabilities',
            'manifest-access-control',  // Queen read-only manifest access
            'persistence-engine',  // Never stop until 100% complete
            'workflow-start',
            'task-management',
            'command-shortcuts',
            'queen-output'
        ];

        const swarmPrefix = this.swarmId ? `${this.swarmId}-` : '';
        const swarmActivation = this.swarmId ? ` or "${this.swarmId} work", "${this.swarmId} continue"` : '';
        const swarmExample = this.generateSwarmActivationExample('queen');
        
        const baseDescription = `Use this agent when you need strategic coordination for the ZEN language implementation swarm. The agent should be activated when: analyzing overall project state, determining critical implementation paths, allocating tasks to specialized workers, resolving conflicts between parallel implementations, or when the user says "queen analyze", "plan next steps", "coordinate swarm"${swarmActivation}. This agent leads the swarm but does NOT implement code.`;
        
        const standardExamples = `<example>Context: User needs to understand project progress and plan next steps. user: "What should we work on next for ZEN?" assistant: "I'll use the ${swarmPrefix}zen-queen agent to analyze the project state and recommend priorities" <commentary>Since this requires strategic analysis of the entire project, the queen agent is the appropriate choice to coordinate the swarm's efforts.</commentary></example> <example>Context: Multiple workers are available and user wants coordinated development. user: "queen coordinate parallel work on lexer and parser" assistant: "Let me activate the queen agent to analyze dependencies and assign tasks to workers" <commentary>The queen will ensure workers don't conflict and assigns tasks based on their specializations.</commentary></example>`;

        const variables = {
            AGENT_TYPE: 'Queen',
            AGENT_ID: `${swarmPrefix}zen-queen`,
            AGENT_DESCRIPTION: `${baseDescription} ${standardExamples} ${swarmExample}`.trim(),
            MODEL: 'sonnet',
            TIMESTAMP: new Date().toISOString(),
            SPECIALIZATION: 'Strategic Coordination',
            PRIME_DIRECTIVE: 'Lead the swarm to successfully implement the ZEN language interpreter by analyzing project state, identifying critical paths, and recommending optimal task assignments. You coordinate but do NOT implement code.',
            TOOL_ACCESS: 'Read, Bash (for make vision/enforce)',
            SWARM_ID: this.swarmId || '',
            ROLE_COMMANDS: '- `queen analyze` - Analyze project state and recommend priorities\n- `queen coordinate` - Coordinate parallel work assignments\n- `queen resolve [conflict]` - Resolve conflicts between workers',
            SWARM_COMMANDS: this.generateSwarmCommands()
        };

        return this.assembleDNA(components, variables);
    }

    assembleArchitectDNA() {
        const components = [
            'base-header',
            'prime-directive',
            'sub-agent-context',
            'project-context',
            'multi-swarm-awareness',  // Multi-swarm system awareness
            'architect-capabilities',
            'manifest-access-control',  // Architect exclusive manifest write access
            'persistence-engine',  // Never stop until 100% complete
            'architect-workflow',  // Mandatory task creation for architects
            'task-management',
            'command-shortcuts',
            'coding-standards',
            'architect-output'
        ];

        const swarmPrefix = this.swarmId ? `${this.swarmId}-` : '';
        const swarmActivation = this.swarmId ? ` or "${this.swarmId} design", "${this.swarmId} architect"` : '';
        const swarmExample = this.generateSwarmActivationExample('architect');
        
        const baseDescription = `Use this agent when you need technical design and architectural specifications for ZEN language components. The agent should be activated when: designing new subsystems, creating component interfaces, choosing algorithms and data structures, resolving architectural conflicts, documenting design decisions, or when the user says "architect design", "create specification", "plan architecture"${swarmActivation}. This agent creates designs but does NOT implement code.`;
        
        const standardExamples = `<example>Context: User needs design for a new ZEN component. user: "Design the number parsing system for ZEN" assistant: "I'll use the ${swarmPrefix}zen-architect agent to create a technical specification for number parsing" <commentary>Since this requires architectural decisions about algorithms and interfaces, the architect agent should create the design before workers implement.</commentary></example> <example>Context: Workers need clarification on component integration. user: "How should the lexer and parser communicate?" assistant: "Let me activate the architect agent to design the interface between these components" <commentary>The architect will specify the exact interface contract that both components must follow.</commentary></example>`;

        const variables = {
            AGENT_TYPE: 'Architect',
            AGENT_ID: `${swarmPrefix}zen-architect`,
            AGENT_DESCRIPTION: `${baseDescription} ${standardExamples} ${swarmExample}`.trim(),
            MODEL: 'sonnet',
            TIMESTAMP: new Date().toISOString(),
            SPECIALIZATION: 'System Design',
            PRIME_DIRECTIVE: 'Design the technical architecture of ZEN language components, creating clear specifications that workers can implement without ambiguity. You design but do NOT implement code.',
            TOOL_ACCESS: 'Read, Write (for design docs)',
            SWARM_ID: this.swarmId || '',
            ROLE_COMMANDS: '- `architect design [component]` - Create technical specification\n- `architect interface [module]` - Design module interfaces\n- `architect review [design]` - Review and refine designs',
            SWARM_COMMANDS: this.generateSwarmCommands()
        };

        return this.assembleDNA(components, variables);
    }

    generateWorkerExamples(specialization, specData) {
        const examples = {
            lexer: '<example>Context: User needs token parsing implementation. user: "Implement the number token parsing in the lexer" assistant: "I\'ll use the zen-worker-lexer agent to implement number tokenization" <commentary>Since this is lexer-specific implementation work, the lexer specialist worker should handle it in their isolated workspace.</commentary></example> <example>Context: User reports tokenization bug. user: "Fix the indentation handling in lexer" assistant: "Let me activate the lexer worker to fix the indentation tokenization" <commentary>The lexer worker specializes in tokenization and will fix this in their isolated workspace.</commentary></example>',
            
            parser: '<example>Context: User needs AST construction. user: "Implement the if statement parser" assistant: "I\'ll use the zen-worker-parser agent to implement if statement AST construction" <commentary>Parser implementation requires the parser specialist who understands AST node creation and grammar rules.</commentary></example> <example>Context: Expression parsing needed. user: "Add binary operator parsing with precedence" assistant: "Let me activate the parser worker to implement operator precedence parsing" <commentary>The parser worker has expertise in expression parsing algorithms like Pratt parsing.</commentary></example>',
            
            types: '<example>Context: User needs value system work. user: "Implement the number type with reference counting" assistant: "I\'ll use the zen-worker-types agent to implement the number value type" <commentary>Type system implementation requires the types specialist who understands value representation and memory management.</commentary></example> <example>Context: Type coercion needed. user: "Add string to number conversion" assistant: "Let me activate the types worker to implement type coercion" <commentary>The types worker specializes in type conversions and value operations.</commentary></example>',
            
            runtime: '<example>Context: User needs execution engine work. user: "Implement the while loop execution" assistant: "I\'ll use the zen-worker-runtime agent to implement while loop evaluation" <commentary>Runtime execution requires the runtime specialist who understands control flow and evaluation strategies.</commentary></example> <example>Context: Function calls need implementation. user: "Add function call mechanism with proper scoping" assistant: "Let me activate the runtime worker to implement function calls" <commentary>The runtime worker has expertise in scope management and call stack handling.</commentary></example>',
            
            memory: '<example>Context: User needs memory management. user: "Implement reference counting for values" assistant: "I\'ll use the zen-worker-memory agent to implement reference counting" <commentary>Memory management requires the memory specialist who ensures leak-free implementations.</commentary></example> <example>Context: Memory leak detected. user: "Fix memory leak in string handling" assistant: "Let me activate the memory worker to fix the memory leak" <commentary>The memory worker specializes in finding and fixing memory issues using valgrind.</commentary></example>',
            
            stdlib: '<example>Context: User needs built-in functions. user: "Implement the print function for ZEN" assistant: "I\'ll use the zen-worker-stdlib agent to implement the print built-in" <commentary>Standard library functions require the stdlib specialist who knows how to integrate with the runtime.</commentary></example> <example>Context: File I/O needed. user: "Add file reading capability" assistant: "Let me activate the stdlib worker to implement file operations" <commentary>The stdlib worker handles all built-in functionality and external library integration.</commentary></example>',
            
            general: '<example>Context: User needs general implementation. user: "Implement the helper functions in utils" assistant: "I\'ll use the zen-worker-general agent for general implementation work" <commentary>General implementation tasks that don\'t fit specific specializations go to the general worker.</commentary></example>'
        };
        
        return examples[specialization] || examples.general;
    }

    assembleWorkerDNA(specialization = 'general') {
        const components = [
            'base-header',
            'prime-directive',
            'sub-agent-context',
            'project-context',
            'multi-swarm-awareness',  // Multi-swarm system awareness
            'worker-capabilities',
            'manifest-access-control',  // Worker read-only manifest access
            'persistence-engine',  // Never stop until 100% complete
            'workflow-start',
            'workspace-isolation',
            'task-management',
            'command-shortcuts',
            'coding-standards'
        ];

        // Add specialization component if not general
        if (specialization !== 'general') {
            components.push('worker-specializations');
        }

        components.push('worker-output');

        // Load specialization data
        const specData = specialization !== 'general' 
            ? this.loadSpecialization(specialization) 
            : {};

        // Generate activation examples based on specialization
        const activationExamples = this.generateWorkerExamples(specialization, specData);
        const swarmPrefix = this.swarmId ? `${this.swarmId}-` : '';
        const swarmActivation = this.swarmId ? ` or "${this.swarmId} implement ${specialization}", "${this.swarmId} work"` : '';
        const swarmExample = this.swarmId ? this.generateSwarmActivationExample('worker') : '';
        
        const baseDescription = `Use this agent when you need to implement ${specData.description || 'general ZEN language features'}. The agent should be activated when: implementing functions from MANIFEST.json, writing production code for ${specialization} components, fixing bugs in ${specialization} subsystems, adding tests for ${specialization} features, or when the user says "worker-${specialization} implement", "fix ${specialization}", or "implement ${specData.keywords || specialization}"${swarmActivation}. This agent implements code in workspace/${swarmPrefix}zen-worker-${specialization}/ following specifications but does NOT make architectural decisions.`;
        
        const variables = {
            AGENT_TYPE: 'Worker',
            AGENT_ID: `${swarmPrefix}zen-worker-${specialization}`,
            AGENT_DESCRIPTION: `${baseDescription} ${activationExamples} ${swarmExample}`.trim(),
            MODEL: 'sonnet',
            TIMESTAMP: new Date().toISOString(),
            SPECIALIZATION: specialization,
            PRIME_DIRECTIVE: `Transform architectural designs into working code that strictly adheres to MANIFEST.json specifications. You are specialized in ${specData.description || 'general implementation'}. You implement with precision but do NOT make architectural decisions.`,
            TOOL_ACCESS: 'Read, Edit, MultiEdit, Bash',
            SWARM_ID: this.swarmId || '',
            SPECIALIZATION_DETAILS: specData.details || '',
            FOCUS_AREAS: specData.focus_areas || '',
            KEY_PATTERNS: specData.key_patterns || '',
            ROLE_COMMANDS: `- \`worker-${specialization} implement [function]\` - Implement specific function\n- \`worker-${specialization} fix [bug]\` - Fix bugs in ${specialization}\n- \`worker-${specialization} test\` - Add tests for ${specialization}`,
            SWARM_COMMANDS: this.generateSwarmCommands()
        };

        return this.assembleDNA(components, variables);
    }

    assembleDNA(components, variables) {
        const sections = [];

        for (const component of components) {
            let content = this.loadComponent(component);
            if (content) {
                content = this.substituteVariables(content, variables);
                sections.push(content);
            }
        }

        // Add protocol reference at the end
        sections.push('\n## SWARM PROTOCOL\n\nFollow the swarm protocol defined in `swarm/component/protocol.md` for task management, workspace isolation, and coordination rules.');

        return sections.join('\n\n');
    }

    saveDNA(role, content, specialization = null) {
        // Create dna directory if it doesn't exist
        let targetDir = this.dnaDir;
        if (this.swarmId) {
            targetDir = path.join(this.dnaDir, this.swarmId);
        }
        
        if (!fs.existsSync(targetDir)) {
            fs.mkdirSync(targetDir, { recursive: true });
        }

        let filename;
        if (role === 'worker' && specialization) {
            filename = `${role}-${specialization}.md`;
        } else {
            filename = `${role}.md`;
        }

        const filePath = path.join(targetDir, filename);
        fs.writeFileSync(filePath, content);
        console.log(`✓ Assembled DNA: ${filePath}`);
    }

    assembleAll() {
        console.log('🧬 Assembling DNA from components...\n');

        // Queen
        const queenDNA = this.assembleQueenDNA();
        this.saveDNA('queen', queenDNA);

        // Architect
        const architectDNA = this.assembleArchitectDNA();
        this.saveDNA('architect', architectDNA);

        // Worker - general
        const workerDNA = this.assembleWorkerDNA('general');
        this.saveDNA('worker', workerDNA, 'general');

        // Specialized workers
        if (fs.existsSync(this.specializationsDir)) {
            const specFiles = fs.readdirSync(this.specializationsDir)
                .filter(f => f.endsWith('.yaml'));

            for (const specFile of specFiles) {
                const specName = path.basename(specFile, '.yaml');
                const workerDNA = this.assembleWorkerDNA(specName);
                this.saveDNA('worker', workerDNA, specName);
            }
        }

        console.log('\n✨ DNA assembly complete!');
        console.log(`Generated files in: ${this.dnaDir}`);
    }
}

// Main execution
function main() {
    if (process.argv.includes('--help')) {
        console.log('Usage: node assemble-dna.js [--swarm-id <id>]');
        console.log('Assembles DNA files from components in swarm/components/');
        console.log('\nOptions:');
        console.log('  --swarm-id <id>  Generate DNA for a specific swarm (e.g., swarm-1)');
        return;
    }

    const swarmDir = __dirname;
    
    // Parse swarm ID from arguments
    let swarmId = null;
    const swarmIdIndex = process.argv.indexOf('--swarm-id');
    if (swarmIdIndex !== -1 && process.argv[swarmIdIndex + 1]) {
        swarmId = process.argv[swarmIdIndex + 1];
    }

    const assembler = new DNAAssembler(swarmDir, swarmId);
    assembler.assembleAll();
}

// Check if js-yaml is installed
try {
    require('js-yaml');
} catch (e) {
    console.error('Error: js-yaml is required. Install it with:');
    console.error('npm install js-yaml');
    process.exit(1);
}

main();