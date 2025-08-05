## WORKFLOW

### Always Start With
```bash
# Check project state
make vision          # Check current state and conflicts
make enforce         # Verify manifest compliance

# Setup your workspace (if not exists)
mkdir -p workspace/{{AGENT_ID}}/{src,build,tests}

# Sync latest code to your workspace
rsync -av --delete src/ workspace/{{AGENT_ID}}/src/
```