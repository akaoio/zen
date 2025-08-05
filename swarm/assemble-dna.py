#!/usr/bin/env python3

import os
import sys
import yaml
import json
from datetime import datetime
from pathlib import Path

class DNAAssembler:
    def __init__(self, swarm_dir):
        self.swarm_dir = Path(swarm_dir)
        self.components_dir = self.swarm_dir / "components"
        self.specializations_dir = self.swarm_dir / "specializations"
        self.dna_dir = self.swarm_dir / "dna"
        
    def load_component(self, name):
        """Load a component file"""
        path = self.components_dir / f"{name}.md"
        if path.exists():
            return path.read_text()
        return ""
    
    def load_specialization(self, name):
        """Load specialization data"""
        path = self.specializations_dir / f"{name}.yaml"
        if path.exists():
            with open(path) as f:
                return yaml.safe_load(f)
        return {}
    
    def substitute_variables(self, content, variables):
        """Replace {{VARIABLE}} placeholders"""
        for key, value in variables.items():
            placeholder = f"{{{{{key}}}}}"
            content = content.replace(placeholder, str(value))
        return content
    
    def assemble_queen_dna(self):
        """Assemble DNA for queen agent"""
        components = [
            "base-header",
            "prime-directive",
            "sub-agent-context",
            "project-context",
            "queen-capabilities",
            "workflow-start",
            "queen-output"
        ]
        
        variables = {
            "AGENT_TYPE": "Queen",
            "AGENT_ID": "zen-queen",
            "TIMESTAMP": datetime.now().isoformat(),
            "SPECIALIZATION": "Strategic Coordination",
            "PRIME_DIRECTIVE": "Lead the swarm to successfully implement the ZEN language interpreter by analyzing project state, identifying critical paths, and recommending optimal task assignments. You coordinate but do NOT implement code.",
            "TOOL_ACCESS": "Read, Bash (for make vision/enforce)"
        }
        
        return self._assemble_dna(components, variables)
    
    def assemble_architect_dna(self):
        """Assemble DNA for architect agent"""
        components = [
            "base-header",
            "prime-directive", 
            "sub-agent-context",
            "project-context",
            "architect-capabilities",
            "workflow-start",
            "coding-standards",
            "architect-output"
        ]
        
        variables = {
            "AGENT_TYPE": "Architect",
            "AGENT_ID": "zen-architect",
            "TIMESTAMP": datetime.now().isoformat(),
            "SPECIALIZATION": "System Design",
            "PRIME_DIRECTIVE": "Design the technical architecture of ZEN language components, creating clear specifications that workers can implement without ambiguity. You design but do NOT implement code.",
            "TOOL_ACCESS": "Read, Write (for design docs)"
        }
        
        return self._assemble_dna(components, variables)
    
    def assemble_worker_dna(self, specialization="general"):
        """Assemble DNA for worker agent with specialization"""
        components = [
            "base-header",
            "prime-directive",
            "sub-agent-context", 
            "project-context",
            "worker-capabilities",
            "workflow-start",
            "coding-standards",
            "worker-output"
        ]
        
        # Add specialization component if not general
        if specialization != "general":
            components.insert(-1, "worker-specializations")
        
        # Load specialization data
        spec_data = self.load_specialization(specialization) if specialization != "general" else {}
        
        variables = {
            "AGENT_TYPE": "Worker",
            "AGENT_ID": f"zen-worker-{specialization}",
            "TIMESTAMP": datetime.now().isoformat(),
            "SPECIALIZATION": specialization,
            "PRIME_DIRECTIVE": f"Transform architectural designs into working code that strictly adheres to MANIFEST.json specifications. You are specialized in {spec_data.get('description', 'general implementation')}. You implement with precision but do NOT make architectural decisions.",
            "TOOL_ACCESS": "Read, Edit, MultiEdit, Bash",
            "SPECIALIZATION_DETAILS": spec_data.get('details', ''),
            "FOCUS_AREAS": spec_data.get('focus_areas', ''),
            "KEY_PATTERNS": spec_data.get('key_patterns', '')
        }
        
        return self._assemble_dna(components, variables)
    
    def _assemble_dna(self, components, variables):
        """Assemble DNA from components"""
        sections = []
        
        for component in components:
            content = self.load_component(component)
            if content:
                content = self.substitute_variables(content, variables)
                sections.append(content)
        
        # Add protocol reference at the end
        sections.append("\n## SWARM PROTOCOL\n\nFollow the swarm protocol defined in `swarm/component/protocol.md` for task management, workspace isolation, and coordination rules.")
        
        return "\n\n".join(sections)
    
    def save_dna(self, role, content, specialization=None):
        """Save assembled DNA to file"""
        self.dna_dir.mkdir(exist_ok=True)
        
        if role == "worker" and specialization:
            filename = f"{role}-{specialization}.md"
        else:
            filename = f"{role}.md"
            
        path = self.dna_dir / filename
        path.write_text(content)
        print(f"✓ Assembled DNA: {path}")
    
    def assemble_all(self):
        """Assemble all DNA files"""
        print("🧬 Assembling DNA from components...\n")
        
        # Queen
        queen_dna = self.assemble_queen_dna()
        self.save_dna("queen", queen_dna)
        
        # Architect
        architect_dna = self.assemble_architect_dna()
        self.save_dna("architect", architect_dna)
        
        # Workers - general and specialized
        worker_dna = self.assemble_worker_dna("general")
        self.save_dna("worker", worker_dna, "general")
        
        # Specialized workers
        for spec_file in self.specializations_dir.glob("*.yaml"):
            spec_name = spec_file.stem
            worker_dna = self.assemble_worker_dna(spec_name)
            self.save_dna("worker", worker_dna, spec_name)
        
        print("\n✨ DNA assembly complete!")
        print(f"Generated files in: {self.dna_dir}")

def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--help":
        print("Usage: python assemble-dna.py")
        print("Assembles DNA files from components in swarm/components/")
        return
    
    swarm_dir = Path(__file__).parent
    assembler = DNAAssembler(swarm_dir)
    assembler.assemble_all()

if __name__ == "__main__":
    main()