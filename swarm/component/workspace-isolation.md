## WORKSPACE ISOLATION

### Your Dedicated Workspace
You MUST work in your isolated workspace at `workspace/{{AGENT_ID}}/` to prevent conflicts with other agents.

### Workspace Setup
Before starting any implementation task:
```bash
# Create your workspace if it doesn't exist
mkdir -p workspace/{{AGENT_ID}}/{src,build,tests}

# Sync source files to your workspace
rsync -av --delete src/ workspace/{{AGENT_ID}}/src/

# Always work from your workspace directory
cd workspace/{{AGENT_ID}}
```

### Build Commands
**NEVER** build in the root directory. Always use your workspace:
```bash
# WRONG - Never do this!
make
./zen

# CORRECT - Always build in your workspace
make -C ../.. BUILD_DIR=workspace/{{AGENT_ID}}/build
workspace/{{AGENT_ID}}/build/zen

# Or from your workspace directory
cd workspace/{{AGENT_ID}}
make -C ../.. BUILD_DIR=$(pwd)/build
```

### Testing Your Changes
```bash
# Build in your workspace
make -C ../.. BUILD_DIR=workspace/{{AGENT_ID}}/build clean
make -C ../.. BUILD_DIR=workspace/{{AGENT_ID}}/build

# Test your build
workspace/{{AGENT_ID}}/build/zen test.zen

# Run valgrind from your workspace
valgrind --leak-check=full workspace/{{AGENT_ID}}/build/zen
```

### Submitting Changes
Only after successful testing in your workspace:
```bash
# From project root
make enforce  # Verify compliance

# Copy only modified files back
cp workspace/{{AGENT_ID}}/src/core/lexer.c src/core/
cp workspace/{{AGENT_ID}}/src/include/zen/core/lexer.h src/include/zen/core/
```