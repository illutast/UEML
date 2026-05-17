# UEML : Universal Extended Markup Language

## Introduction
UEML is an stronger alternative to the famous **XML** language, with its distinct syntax and unique features that most static language doesnt have.
It is designed to bridge the gap between static data formats (XML / JSON), and a concise property syntax to reduce verbosity while maintaining high readability for users.

The main reason that UEML was developed is to make markup language more modern and readable, with some features that are useful for use.

Currently the parser and the compiler is only available in C++, if you want to contribute your parser / compiler, please send a request to me as stated
in GitHub.

## Naming rules in UEML

- Names cannot be written with space, as it seperates the name entirely.
- You can write numbers into the scope name but not in the start of the name.
- The name MUST be alphanumerical, with exceptions being `_`.
- Names are case-sensitive, which means `Hello` is entierly different from `hello`.


## Data types in UEML

### `Number`
The section is self-explanatory. It represents a number, either can be a decimal, or a integer.
It can be typed just like other languages, such as `123456` and `192035.238`.

The data type has no numerical & decimal limits, and all of them are represented as string to the Parser.

### `String`

The section is also self-explanatory. It represents a string.
It can be typed as `".."`, such as `"Hello world!"` and `"Funny"`.

There also exists **Multiline strings**, which is a unique type of string that allows you to type multiline strings.
It can be typed as `'..'`, such as:
```
'
Hello world,
goodbye world.
'

'Hello world,
goodbye world.'

'
Hello world,
goodbye world.'

'Hello world,
goodbye world.
'
```

Strings & multistrings cannot have operators that are reserved for UEML to work. To use it, you must use the **escape character**.



### `List`
The section is also self-explanatory. The data type represents a list. What makes it unique is the data type allows you to mix different data types into a single `List` object.

The List object also allows you to make nested `List` or `Maps`.
It can be typed as `{...}` such as `{1, 2, "hello", 4}`, `{"thing", {"2", 3}}`.

### `Maps`

An unique type of the datatype `List`, that allows you to set keys & values.
It has a different syntax. A key & a value is represented as `(key)=(value)`,
which is called a **value pair**.

Every value pair is seperated with a comma (`,`). This is an example of maps:
```
[
    Number = 1,
    List = {1, 2, "Three!"}
]
```

## Features in UEML

### `Scopes` (`Tags`) & `ScopeProperties` (`TagProperties`)
#### `Scopes` (`Tags`)
A `Scope` (or called `Tag`) object is a markup construct used to define the structure, meaning, and boundaries of data within an UEML document. You can define your own `Scope` with a name.

Unlike traditional XML, UEML scopes allow for complex data structures to be passed directly into the tag via `ScopeProperties`.

A normal scope follows a format like this:
```
<(Scope name) [Scope properties]>
    ...
</(Scope name)> 
```
where at `<(Scope name) [Scope properties]>`, represents the beginning of a `Scope`, `(Scope name)`
represents the name of the scope, `[Scope properties]` represents the properties of the scope, which will 
be explained later.

At `</(Scope name)>` represents the end of a `Scope`.

At `...` is mostly called a `Body`.

`(Scope name)` follows the UEML Name rules.

#### `ScopeProperties` (`TagProperties`)
`ScopeProperties` are optional properties defined within square brackets `[]`, immediately following the scope name. They function as a Map specific to that scope instance. 

Example:
```
<ScopeName [Key1 = Value1, Key2 = Value2]>
    ...
</ScopeName>
```

Always note that a `ScopeProperties` is optional, which means that `Scopes` does not need to always have a `ScopeProperties` object.

#### Types of `Scopes`
UEML supports 3 types of scopes: `NormalScope`, `DummyScope` and `SelfClosingScope`.
##### `NormalScope`
The standard type of scope used for nesting other scopes or text content. It always requires an 
explicit closing scope.
It is typed as `<(Scope name) [Scope properties]> ...  </(Scope name)>`

```
<Container [Type = 3]>
    Nested content or text goes here...
</Container>
```

##### `SelfClosingScope`

Used for elements that do not contain nested data or children. These are terminated with a trailing slash `/`.
Also supports `ScopeProperties`.
It is typed as `<(ScopeName) [ScopeProperties] />`.
```
<Data [This = "here"] />
```

##### `DummyScope`
A unique UEML scope used for grouping elements or variables without introducing a named node into the final data tree. This is useful for organizational logic that should not affect the parser's output hierarchy.

```
<>
    <Step1>
        Cook the egg.
    </Step1>
    <Step1>
        Present the egg.
    </Step2>
</>
```

DummyScope also allows you to have `ScopeProperties`.

```
<[Name = "DummyScope", S = 1]> Hello! </>
```

### `Comments`
The inline comment syntax is a bit similar to XML. Comments do not represent anything, so it will be eventually removed
upon compilation, because it only represents notes.

The inline comment is typed as `<!-- ... --!>`.

Example of a inline comment : `<!-- Comment... --!>`

There are also multi line comments, which is typed as `<!-~ ... ~-!>`.

Example of a multiline comment:
```
<!-~ 
Myself does not represent anything,
so that Parser would remove me!
~-!>
```

### `Variables`

This feature is very unique because it does not exist in XML.
This feature is made to provide a way to store and reuse data dynamically. Variables allow you to define values 
once and reference them throughout the document, reducing redundancy and making configurations more maintainable.

Variables are identified with `$` (dollar sign) at the start. If you want to access the variable you can just use
the variable name with the `$` (dollar sign) at the start to access it.

Variables name follows the UEML naming rules.

There are two primary ways to declare/modify
a variable: within `ScopeProperties` or within the `Body`.

#### Within `ScopeProperties`

You can initialize a variable directly inside the property brackets of the beginning of the scope.
And, `ScopeProperties` can also have values defined as variables.

However, it would not work if it was in a `Map`, it is progammed to strictly defined in `ScopeProperties`.

```
<Header [$ThemeColor = "Blue", $Version = 1.0]>
    Here goes $ThemeColor !
</Header>
```

#### Within the `Body`

You can use the double-bracket syntax `[[...]]`.

```
<Project>
    [[ $InternalID = 5050 ]]
    [[ $Status = "Draft" ]]
</Project>
```

Variables in UEML follows the lexical scoping rules: `LocalVariable` and `GlobalVariable`.

`LocalVariable` is a variable that is defined within a scope and is only available to nested scopes and the scope
itself. When the scope with the `LocalVariable` ends, it removes itself, which means you cannot use the variable
outside the scope.

However, `GlobalVariable` is opposite to `LocalVariable`, which is defined outside all scopes, is available to every
scope in the document.

An complete example for variable:

```
[[ $AppName = "Lorem Ipsum" ]]

<AppInfo [Title = $AppName]>
    Welcome to $AppName.
</AppInfo>
```

### Escape characters

To ensure the parser doesn't confuse plain text with UEML's reserved symbols, the backslash \ is used to escape reserved characters.
The list of escapable characters includes `\<`, `\>`, `\$`, `\[`, `\]`, `\"`, `\@` and `\=`.

```
<D>
    To define a variable, use the \$ symbol.
    To show a tag, write \<TagName\>.
</D>
```

### Trimming

Unlike XML, UEML has a bit different formatting / trimming method as the compiler removes every tab and spaces and present it to the Parser, such as:

```
<B>
    Hello world!
        This is with tab.
</B>
```

The compiler trims the result to:
```
Hello world!
This is with tab.
```

So you may want to use `\@`, to make the formatter knows that this line does not need additional formatting, it will skip that line and formats
every line without the `\@` at the start.

## Complete UEML example

```
<Process [ Content = [
    Title = "New Content",
    AspectList = {800, 600},
    Version = 12
] , $Variable = "This Variable" ]>
    <!-- This is an comment --!>
    <!-~
    This is an multiline comment
    It Works!
    ~-!>
    <Button [Size = {100, 100}, OnClick = "Func()"]>
        Click <Bold> \" Me! \" </Bold>
    </Button>
    This is the body
    \@ This is intended spacing
    You can reference variable via \$. This is an example: $Variable.
    <SectionBreak [Height = 2.1]/>
</Process>
```
