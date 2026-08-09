import fs from "node:fs";

// CWD-relative cleanup — npm scripts always run from project root
const drop = ["tmp", "radata", "radatatest", "data", "data.json"];

// stats.* are the runtime writer's droppings; before 1.0.45 they landed here
// instead of tmp/, and a repo full of them is what made the bug visible.
for (const name of fs.readdirSync(".")) {
  if (name.startsWith("stats.")) {
    try {
      fs.rmSync(name, { force: true });
    } catch (e) {}
  }
}
for (const d of drop) {
  try {
    fs.rmSync(d, {
      recursive: true,
      force: true,
      maxRetries: 10,
      retryDelay: 200,
    });
  } catch (e) {}
}
for (const name of fs.readdirSync(".")) {
  if (name.startsWith("radatatest-") || name.startsWith("radata-")) {
    try {
      fs.rmSync(name, {
        recursive: true,
        force: true,
        maxRetries: 5,
        retryDelay: 200,
      });
    } catch (e) {}
  }
}
