# UEML

<p align="center">
  <img width="481" height="218" alt="UEMLLogo" src="https://github.com/user-attachments/assets/5d31ef05-9c42-495f-91dc-5bdc8cc7262a" />
</p>
<p align="center">
  <b>Better dynamic alternative to XML and HTML</b>
</p>

UEML is an open-source markup language, distributed under Apache License 2.0.

UEML is an alternative to XML / HTML, but instead of statically typed, UEML is a **dynamic declarative markup language** with a lot of new features including:

- Variables and variable references
- Lexical scoping of variables (local & global)
- Lower redundancy
- Better formatting & trimming (trims every space and tabs inside bodies for better readability, allows preserving intended tabs & spaces via `\@`)
- Expanded first-class value (data) types (Number (integer / float), Strings, Multi-strings, List and Maps)
- Compilation -> parsing process

UEML was created to provide the readability of XML, but with more features that is useful for those who want to make their code atleast have more readability than XML.

UEML is available through C++ only as a library, but if you want to provide your own parser and compiler for a specific platform / progamming language, please DM me via Discord (illutast) to add into this repository.

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
# C++ library requirements

- Requires atleast `C++17`
- Requires Boost library to be installed, in order to use logging features from the compiler / parser (Boost.Log)
- Arguments to compile are `-DBOOST_LOG_DYN_LINK -lboost_log-mt -lboost_log_setup-mt -lboost_thread -lpthread`
- Library is also header only so you don't need to have a external linking
