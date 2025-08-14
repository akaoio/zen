# zen Language Specification

> zen is a lightweight, mobile-friendly, markdown-compatible scripting language designed for clarity, minimal syntax, and strong expressiveness. Ideal for embedding in documents, apps, AI workflows, or DSLs.
> 
> Although not the most powerful language, it is the one that is purest and closest to human natural language. To me, it is the most beautiful programming language.

*- Nguyen Ky Son - Aug 1st 2025*

---

## 1. Variables, Basic Types, Operators

Variables are declared using `set`:

```zen
set made "in Vietnam",
    by "Nguyen Ky Son", 
    age 40
set pi 3.14
set active true
set empty null
```

Values can be string, number, boolean, null, array, object, or result of expression.

### Variable Scope

Variables can be local or global:

```zen
set price 1000           // global var
function plan price      // local var
    print "price is " + price
    
plan 900                 // call plan function with param price = 900
```

This prints "price is 900" because `price` in `plan` is local.

### Operators

- `=` (equivalent to `==` in most other languages)
- `<=`, `>=`
- `+=`, `-=`, `*=`, `/=`
- `!=`, `!==` (equivalent to `!=` and `!==` in JS)
- `&`, `|`, `!` (logical AND, OR, NOT)

IMPORTANT: `=` for comparision insted of `==`

---

## 2. Data Structures

### 2.1 Arrays

**Inline Arrays (Preferred):**
```zen
set scores 1, 2, 3
```

**Multiline Arrays:**
```zen
set items
    "pen",
    "book", 
    "lamp",
```

- Elements can be literals, variables, or expressions
- Commas `,` required to separate elements
- Indentation required in multiline format

### 2.2 Objects

**Inline Objects (Preferred):**
```zen
set user name "Alice", age 30, active true
```

**Multiline Objects:**
```zen
set config
    debug true,
    retries 3,
    timeout 1000
```

- No colons `:`, no curly braces `{}`
- Comma `,` required between entries
- Each entry is a key-value pair

### 2.3 Nested Structures

```zen
set scores 8, 9, 10
set profile name "Linh", scores scores, active true
```

### 2.4 Multidimensional Arrays

**Flat matrix:**
```zen
set matrix 1, 2, 3,
           4, 5, 6,
           7, 8, 9,
```

**Nested matrix:**
```zen
set matrix
    [1, 2, 3],
    [4, 5, 6], 
    [7, 8, 9],
```

**Accessing elements:**
```zen
set x matrix[0][1]      // 2
set y matrix[1]
set z y[2] // 6
```

---

## 3. Functions

```zen
function greet name
    return "Hello " + name

function add a b
    return a + b
```

- Arguments are space-separated
- No parentheses required around parameters

### 3.1 Recursive Functions

```zen
function factorial n
    if n = 0
        return 1
    else
        return n * factorial n - 1

// Tail-recursive style:
function fact_tail n acc
    if n = 0
        return acc
    else
        return fact_tail n - 1, acc * n
```

### 3.2 Lambda & Higher-Order Functions

```zen
set doubled map numbers (x => x * 2)
set evens filter numbers (x => x % 2 = 0)
```

### 3.3 Generators

```zen
function* fib
    set a 0
    set b 1
    while true
        yield a
        set a b
        set b a + b
        if b > 10 then break
```

---

## 4. Control Flow

### 4.1 Conditionals

**Important:** In zen, we use `=` to compare (not assign):

```zen
if score = 100 then print "Wowww"
if score >= 90
    return "Excellent"
elif score >= 70
    return "Good"
else
    return "Try again"
```

**Ternary Operator:**
```zen
set result age >= 18 ? "adult" : "minor"
```

### 4.2 Loops

**Repeat N times:**
```zen
loop 100 times
    print "I am sorry..."
```

**For in range:**
```zen
for i in 1 .. 5
    print i
    if i = 4 then break
```

**For over array:**
```zen
for item in items
    print item
```

**For with index:**
```zen
for i, name in names
    print i + ": " + name
```

**While:**
```zen
set count 0
while count < 3
    print count
    set count count + 1
```

---

## 5. Object-Oriented Programming

```zen
class Animal
    constructor name
        set self.name name

    speak
        return self.name + " makes a sound"

class Dog extends Animal
    constructor name breed
        super name
        set self.breed breed

    speak
        return self.name + " says woof!"
    
    private secret message
        return "my top secret: " + message
```

---

## 6. Error Handling

```zen
function divide a b
    try
        if b = 0
            throw "Divide by zero"
        return a / b
    catch err
        return "Error: " + err
```

---

## 7. Advanced Features

### 7.1 Destructuring & Spread

**Object Destructuring:**
```zen
destruct user name username, age userage
```

**Array Destructuring:**
```zen
destruct items first, second, ...rest
```

**Spread Arrays:**
```zen
set extended ...items, "new"
```

**Spread Objects:**
```zen
set merged ...user, country "VN"
```

### 7.2 Template Literals

```zen
set html `
<div>
  <h1>{{name}}</h1>
  <p>Age: {{age}}</p>
</div>
`
```

### 7.3 Regular Expressions

```zen
set email_pattern /^[^\s@]+@[^\s@]+\.[^\s@]+$/
set valid email_pattern.test "elon@x.com"
```

---

## 8. Built-in Operations

### 8.1 String Operations

```zen
set text "Hello World"
set upper text.upper
set parts text.split " "
set joined parts.join "-"
```

### 8.2 Date & Time

```zen
set now Date.now
set past Date "1990-01-01"
set years now.diff past "years"
```

---

## 9. File I/O & Data Persistence

### 9.1 Basic File Operations

```zen
set phones File.read "phones.csv"
set doc File.read "readme.md"

File.write "output.txt" "Hello World"
File.write "result.json" json.stringify users
File.write "result.yaml" yaml.stringify settings
```

### 9.2 Serialization

```zen
set user name "Alice", age 30

set j json.stringify user
set y yaml.stringify user

set u1 json.parse j
set u2 yaml.parse y
```

### 9.3 Database-like Operations

Use `get` and `put` on JSON/YAML files like a database:

```zen
set name get "./data/students.json".alice.name
set what "is-going-on.yaml"
set answer get what.boss.is_watching

put what.me.working true

set location 123, 456
set project
    name "zen",
    github "akaoio/zen",
    draft true

put what.count 1, 2, 3                          // array
put what.why because true, and "whynot", have "fun"  // object
put what.me.where location
put what.me project
put what.me.project.release false

// Cross-file linking
put "alice.json".scores "../scores.yaml".alice
put "alice.yaml".address.office "@ ../addresses.json office.alice"
set addr get "alice.yaml".address.office
```

**Behavior:**
- `get` on non-existent variable → returns null or throws (configurable)
- `put` to non-existent path → auto-creates nested keys
- `put` to file → writes to file, auto-creates if needed
- `get` from file → lazy loads if not cached

---

## 10. Async Programming

```zen
async function fetch url
    set res await http.get url
    return res.json
```

---

## 11. Modules & Imports

### 11.1 Importing

```zen
import propA newA, propB newB from "config.json"
import description from "data.yaml" 
import "students.json"
import Tasks                    // equivalent to `import "Tasks.md"`
import Tasks tasks              // or import "Tasks.md" tasks
set teachers import "teachers.yaml"
```

When importing `.json` or `.yaml` files, content is automatically parsed into objects.

### 11.2 Exporting

```zen
export function greet name
    return "Hello " + name

export set version "1.0.0"

export class Tool
    method use
        print "Using tool"

set a 15, b 20, c 30
export a, b, c                  // batch export
export a A, b B, c C            // export with aliases

// Conditional export
if environment.mode = "development"
    export devHelper
else
    export prodHelper
```

- No `export default` - everything is named export
- `export` goes before `function`, `class`, or `set`

---

## 12. Formal Logic

zen supports formal logic with minimalist syntax:

### 12.1 Axioms and Theorems

```zen
axiom A1
    all x in N
        if even x
            then divisible x 2

axiom A2
    even 4

theorem T1
    divisible 4 2
```

### 12.2 Proofs

```zen
prove T1 from A1, A2

// Gödel-style Self-Reference
theorem Godel
    not prove Godel
```

**Rules:**
- All logical constructs use plain keywords: `if`, `then`, `not`, `all`, `exists`, `prove`, `from`
- No special symbols like `→`, `∧`, `∀`
- Indentation defines structure
- Logic is first-class for formal derivation and meta-reasoning

---

## 13. Markdown Integration

### 13.1 Code Blocks in Markdown

zen can be embedded in Markdown using standard code blocks:

````markdown
```zen
set name "An"
print "Hello " + name
```
````

### 13.2 HTML Comment Style

zen also supports HTML-style comments for tighter integration:

**Inline:**
```html
Hello <!-- zen print "world" -->!
```

**Multiline:**
```html
<!-- zen
set a 10
set b 20
print a + b
-->
```

### 13.3 Native zen Files

**File extensions:** `.zn` and `.zen`

- zen code is treated natively without ```` ```zen ``` ```` requirement
- Markdown code blocks require ```` ```markdown ``` ```` or ```` ```md ``` ````
- ```` ```zen ``` ```` code blocks inside native files are treated as plain text

---

## 14. Comments

**Single-line:**
```zen
// This is a comment
set x 10 // inline comment
set text "string" # This is also a comment
```

**Multi-line:**
```zen
/*
  This is a multi-line comment
  that spans multiple lines
*/
set y 20
```

---

## 15. Language Rules & Conventions

### Key Principles

- **Minimal syntax:** No need for `{}`, `[]`, or `:` in most cases
- **Mobile-friendly:** Designed for ease of typing on mobile devices
- **Comma-separated:** Use `,` between items in arrays/objects
- **Indentation matters:** Multiline blocks use indentation for structure
- **Natural language flow:** Syntax resembles natural language patterns

### File Extensions

- `.zen` or `.zn` for native zen files
- zen code blocks in `.md` files must use ```` ```zen ``` ````