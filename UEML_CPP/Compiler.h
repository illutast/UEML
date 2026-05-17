#ifndef UEMLCOMPILER_H
#define UEMLCOMPILER_H

#include "StringHex.h"
#include "FileHex.h"
#include <boost/log/trivial.hpp>
#include <queue>
#include <fstream>
#include <filesystem>

/**
* @brief   Compiles the UEML language file into the binary format string.
* @warning Requires the Output pointer to be specified, or throws an fatal error.
*/
inline void UEMLCompileSource(std::u32string Source, UEMLHexademicalString* Output)
{
    if (!Output)
    {
        BOOST_LOG_TRIVIAL(fatal) << "UEMLcompilecheck : Output pointer is undefined";
        return;
    }
    rglInitHexString(Output);

    rglHexStringWriteAdvance(Output, {0x55,0x45,0x4D,0x4C}); //* UEML (ascii)

    rglHexStringWriteAdvance(Output, {0x00,0x00,0x00,0x00}); //* make space

    uint64_t Cursor = 0;

    // checks if the character follows the naming rules
    auto IsUEMLName = [](char Char)
    {
        return isalnum(Char) || Char == '_';
    };

    // reads the source text from [Start, End)
    auto ReadFrom = [&](uint64_t Start, uint64_t End)
    {
        return std::u32string_view(Source.begin() + Start, Source.begin() + End);
    };

    // Peek PeekCurChar() character
    auto PeekCurChar = [&]()
    {
        return Source[Cursor];
    };

    // Move cursor by N character
    auto MoveCursor = [&](uint64_t N)
    {
        Cursor += N;
    };

    // checks if is bounds
    auto InBounds = [&]() -> bool
    {
        return Cursor < Source.size();
    };

    // skips the spcaes and newlines until it reaches the character that isnt a space/newline
    auto SkipSpaces = [&]()
    {
        auto is_space = [](char32_t c) {
            return c == U' ' || c == U'\n' || c == U'\r' || c == U'\t' || c == U'\v' || c == U'\f';
        };
        while (Cursor < Source.length() && is_space(Source[Cursor])) MoveCursor(1);
    };
    
    // checks if a char exists in those string
    auto IfCharExistInStr = [](char32_t Character, std::u32string Str)
    {
        return Str.find(Character) != std::u32string::npos;
    };

    // write the u32 string (WITH string case)
    auto WriteString = [&](uint64_t Start, uint64_t End)
    {
        rglHexStringWriteAdvance(Output, {0x00, 0x03}); // write str begin
        if (Start > End || Start >= Source.length() || End >= Source.length())
        { // safe bounds handling for empty strings
            rglHexStringWriteAdvance(Output, {0xFF, 0x03});
            return;
        }

        for (uint64_t i = Start; i <= End; i++)
        {
            char32_t c = Source[i];
            if (c == '\\')
            {
                if (i == End)
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In string, expected next character in \\, got none";
                    return;
                } else
                {
                    char32_t next_c = Source[i + 1];
                    if (IfCharExistInStr(next_c, U"<>$[]\"'="))
                    {
                        c = next_c;
                        i++; // process escaped char & step over backlash
                    } else if (next_c == '@') // the user wants space in their string
                    {
                        c = ' ';
                        i++; // process space and step over backslash
                    }
                }
            } else if (IfCharExistInStr(c, U"<>$[]\"'="))
            {
                if (c == '$' && i == Start) {
                    // safely allow the $ prefix to exist natively for Map Variables Definitions 
                } else {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In string, found an invalid character: " << (char)c;
                    return;
                }
            }
            unsigned char b1 = (c >> 24) & 0xFF; // highest byte
            unsigned char b2 = (c >> 16) & 0xFF;
            unsigned char b3 = (c >>  8) & 0xFF;
            unsigned char b4 =  c        & 0xFF; // lowest byte

            rglHexStringWriteAdvance(Output, {b1,0x04, b2, 0x04, b3,0x04, b4, 0x04});
        }
        rglHexStringWriteAdvance(Output, {0xFF, 0x03}); // write str end
    };

    
    // write the u32 string (WITHOUT string case)
    auto WriteStringWO = [&](uint64_t Start, uint64_t End)
    {
        if (Start > End || Start >= Source.length() || End >= Source.length()) return;
        for (uint64_t i = Start; i <= End; i++)
        {
            char32_t c = Source[i];
            if (c == '\\')
            {
                if (i == End)
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In string, expected next character in \\, got none";
                    return;
                } else
                {
                    char32_t next_c = Source[i + 1];
                    if (IfCharExistInStr(next_c, U"<>$[]\"'="))
                    {
                        c = next_c;
                        i++;
                    }
                }
            } else if (IfCharExistInStr(c, U"<>$[]\"'="))
            {
                if (c == '$' && i == Start)
                {
                    // do nothing for catch
                } else {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In string, found an invalid character: " << (char)c;
                    return;
                }
            }
            unsigned char b1 = (c >> 24) & 0xFF; 
            unsigned char b2 = (c >> 16) & 0xFF;
            unsigned char b3 = (c >>  8) & 0xFF;
            unsigned char b4 =  c        & 0xFF; 

            rglHexStringWriteAdvance(Output, {b1,0x04, b2, 0x04, b3,0x04, b4, 0x04});
        }
    };

    // similar to WriteString, but instead of cursors its a plain string
    auto WriteStringDirect = [&](const std::u32string& Str)
    {
        rglHexStringWriteAdvance(Output, {0x00, 0x03}); // write str begin
        for (uint64_t i = 0; i < Str.size(); i++)
        {
            unsigned char b1 = (Str[i] >> 24) & 0xFF; // highest byte
            unsigned char b2 = (Str[i] >> 16) & 0xFF;
            unsigned char b3 = (Str[i] >>  8) & 0xFF;
            unsigned char b4 =  Str[i]        & 0xFF; // lowest byte

            rglHexStringWriteAdvance(Output, {b1, 0x04, b2, 0x04, b3, 0x04, b4, 0x04});
        }
        rglHexStringWriteAdvance(Output, {0xFF, 0x03}); // write str end
    };

    // similar to WriteStringWO, but instead of cursors its a plain string
    auto WriteStringDirectWO = [&](const std::u32string& Str)
    {
        for (uint64_t i = 0; i < Str.size(); i++)
        {
            unsigned char b1 = (Str[i] >> 24) & 0xFF; // highest byte
            unsigned char b2 = (Str[i] >> 16) & 0xFF;
            unsigned char b3 = (Str[i] >>  8) & 0xFF;
            unsigned char b4 =  Str[i]        & 0xFF; // lowest byte

            rglHexStringWriteAdvance(Output, {b1, 0x04, b2, 0x04, b3, 0x04, b4, 0x04});
        }
    };

    // formats the string
    auto FormatString = [](std::u32string_view Str) -> std::u32string
    {
        if (Str.empty()) return U"";

        auto IsSpace = [](char32_t c) {
            return c == U' ' || c == U'\n' || c == U'\r' || c == U'\t' || c == U'\v' || c == U'\f';
        };

        size_t S = 0;
        size_t E = Str.size();
        while (S < E && IsSpace(Str[S])) S++;
        while (E > S && IsSpace(Str[E - 1])) E--;

        if (S < E && (Str[S] == '\'' || Str[S] == '"')) S++;
        if (E > S && (Str[E - 1] == '\'' || Str[E - 1] == '"')) E--;

        std::u32string FinRes;
        std::u32string RawLine;

        auto ProcessLine = [&](const std::u32string& line)
        {
            if (line.empty()) return;

            size_t firstNonSpace = line.find_first_not_of(U" \t");
            if (firstNonSpace == std::u32string::npos) return;

            std::u32string content;
            
            if (line.substr(firstNonSpace, 2) == U"\\@")
            {
                content = line.substr(firstNonSpace + 2); 
            }
            else
            {
                size_t s = firstNonSpace;
                size_t e = line.size();
                while (e > s && (line[e - 1] == U' ' || line[e - 1] == U'\t')) e--;
                content = line.substr(s, e - s);
            }

            std::u32string Proc;
            for (size_t i = 0; i < content.size(); ++i)
            {
                if (content[i] == U'\\' && i + 1 < content.size())
                {
                    Proc.push_back(content[++i]);
                }
                else
                {
                    Proc.push_back(content[i]);
                }
            }

            if (!FinRes.empty()) FinRes.push_back(U'\n');
            FinRes.append(Proc);
        };

        for (size_t i = S; i < E; i++)
        {
            if (Str[i] == U'\n' || Str[i] == U'\r')
            {
                ProcessLine(RawLine);
                RawLine.clear();
                if (Str[i] == U'\r' && i + 1 < E && Str[i + 1] == U'\n') i++; //crlf
            }
            else
            {
                RawLine.push_back(Str[i]);
            }
        }
        ProcessLine(RawLine);

        return FinRes;
    };

    enum class ValueType
    {
        Number,
        Map,
        String,
        MultiString,
        List,
        VariableRef,
        Any // to read either one of four values and write there
    };

    auto IsValidNumber = [&](char32_t Chr)
    {
        return isdigit(Chr) || Chr == '.';
    };

    // Reads the value in the source and translate it into binary bytes
    std::function<void(ValueType, bool)> ReadValue  = [&](ValueType Type, bool IsCompilingProperties)
    {
        // resolve each type by redirecting Type
        if (Type == ValueType::Any)
        {
            if (IsValidNumber(PeekCurChar()))         Type = ValueType::Number;
            else if (PeekCurChar() == '"')            Type = ValueType::String;
            else if (PeekCurChar() == '\'')           Type = ValueType::MultiString;
            else if (PeekCurChar() == '{')            Type = ValueType::List;
            else if (PeekCurChar() == '[')            Type = ValueType::Map;
            else if (PeekCurChar() == '$')            Type = ValueType::VariableRef;
            else
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError: Unknown structural type header: " << (char)PeekCurChar();
                return;
            }
        }

        switch (Type)
        {
            case ValueType::Number:
            {
                if (IsValidNumber(PeekCurChar())) // if its a number
                {
                    // 1.234, 12562, .236
                    uint64_t NumberCp = Cursor;
                    while (InBounds() && IsValidNumber(PeekCurChar())) MoveCursor(1); // move until reach the end of num
                    rglHexStringWriteAdvance(Output, {0x00, 0x04}); // write number open, then write data, then close
                    WriteStringWO(NumberCp, Cursor - 1);
                    rglHexStringWriteAdvance(Output, {0xFF, 0x04}); // write the number close bytecode
                }
                break;
            }
            case ValueType::String:
            {
                // "LOL hi",
                /*
                'H

                ello'
                */
                if (PeekCurChar() == '"') // check if its a inline string
                {
                    MoveCursor(1); // we want to see the data and not to include the quotes
                    uint64_t StringCp = Cursor;
                    while (InBounds() && PeekCurChar() != '"') MoveCursor(1); // move until reach the end of string
                    WriteString(StringCp, Cursor - 1); // cursor - 1 because it iterates <= end
                    if (InBounds() && PeekCurChar() == '"') MoveCursor(1); // move past the "
                }
                break;
            }
            case ValueType::MultiString:
            {
                if (PeekCurChar() == '\'') // check if its a multi string
                {   
                    // we will use format string to handle the quotes
                    uint64_t StringCp = Cursor;
                    MoveCursor(1); // move first to prevent softlock in while loop
                    while (InBounds() && PeekCurChar() != '\'') MoveCursor(1); // move until reach the end of multi string
                    WriteStringDirect(FormatString({Source.begin() + StringCp, Source.begin() + Cursor + 1})); // let its handle its own thing (handles for us the quotes case)
                    if (InBounds() && PeekCurChar() == '\'') MoveCursor(1); // move past the '
                }
                break;
            }
            case ValueType::List:
            {
                // {1, 2, "hi!"}
                if (PeekCurChar() == '{') // check if its actually a list
                {
                    rglHexStringWriteAdvance(Output, {0x00, 0x05}); // write the list opening bytecode
                    MoveCursor(1); // move past the { to get to the value
                    while (InBounds())
                    {
                        SkipSpaces();
                        ReadValue(ValueType::Any, false); // recursive reading the value
                        SkipSpaces();
                        if (PeekCurChar() == '}') // check if it ended first!
                        {
                            MoveCursor(1); // skip the bracket
                            break;
                        }
                        else if (PeekCurChar() == ',') // if not, check if its a comma
                        {
                            MoveCursor(1); // move to read the next data
                        } else
                        {
                            // log fatal
                            BOOST_LOG_TRIVIAL(fatal) << "UEMLCompilerError : In List, expected a comma or a list ending (}), got " << PeekCurChar();
                            return;
                        }
                    }
                    rglHexStringWriteAdvance(Output, {0xFF, 0x05}); // write the list close bytecode
                };
                break;
            }
            case ValueType::Map:
            {
                // [ Hello = "Hello world" , G = .25]
                //* note : variable definition code also exists here because scope properties is also a map
                if (PeekCurChar() == '[') // check if its actually a map
                {
                    rglHexStringWriteAdvance(Output, {0x00, 0x06}); // writes the map opening bytecode
                    MoveCursor(1); // move past the [ to get to the key, and then get to the value
                    while (InBounds())
                    {
                        SkipSpaces();

                        // writes the key

                        uint64_t KeyCp = Cursor;

                        if (InBounds() && PeekCurChar() == '$') MoveCursor(1);

                        while (InBounds() && IsUEMLName(PeekCurChar())) MoveCursor(1); // advances until reaches the end of the key

                        if (IsCompilingProperties && ReadFrom(KeyCp, KeyCp + 1)[0] == '$') // check if the key is a variable definition and in Properties
                        {
                            WriteString(KeyCp, Cursor - 1); // writes the STR_LITR (a variable definition, lend it to the parser)
                        } else if (ReadFrom(KeyCp, KeyCp + 1)[0] == '$') // else, that means user is defining variable in a non-scope-properties map
                        {
                            BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In Map, variable definition in a non-scope-properties map is not allowed here";
                            return;
                        } else // its not a variable definition (its a normal key)
                        {
                            WriteString(KeyCp, Cursor - 1); // writes the STR_LITR (a normal key)
                        }

                        SkipSpaces(); // skips the space to reach =
                        if (PeekCurChar() == '=') // safety check to proceed
                        {
                            MoveCursor(1);
                            SkipSpaces(); // to reach the value
                        }
                        else
                        {
                            BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In Map, expected an assignment operator (=), got " << PeekCurChar();
                            return;
                        }

                        ReadValue(ValueType::Any, false); // recursive reading

                        SkipSpaces();
                        
                        if (PeekCurChar() == ']') // check if it ended first!
                        {
                            MoveCursor(1); // skip the bracket
                            break;
                        } else
                        // if not, check if its an comma for safety
                        if (PeekCurChar() == ',')
                        {
                            MoveCursor(1);
                        }
                        else
                        {
                            BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In Map, expected an comma or a map ending (]), got " << PeekCurChar();
                            return;
                        }
                    }
                    rglHexStringWriteAdvance(Output, {0xFF, 0x06}); // writes the map ending bytecode
                }
                break;
            }
            case ValueType::VariableRef:
            {
                if (PeekCurChar() == '$') // check if its actually a variable reference
                {
                    rglHexStringWriteAdvance(Output, {0x00, 0x07}); // writes the variable ref opening bytecode
                    uint64_t VariableNameCp = Cursor; // set here to include the $
                    MoveCursor(1); // bypass the $ to prevent confusing IsUemlname
                    while (InBounds() && IsUEMLName(PeekCurChar())) MoveCursor(1);

                    auto VariableName = ReadFrom(VariableNameCp, Cursor);
                    if (VariableName == U"") // means that user definetly left only $
                    {
                        BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : Variable reference found no name (The compiler only found $)";
                        return;
                    }
                    WriteStringWO(VariableNameCp, Cursor - 1);
                    rglHexStringWriteAdvance(Output, {0xFF, 0x07});
                }
                break;
            };  
            default: break;
        }
    };

    // Main loop for scopes
    while (InBounds())
    {
        if (PeekCurChar() == '<')
        {
            bool IsClose = ReadFrom(Cursor, Cursor + 2) == U"</"; // boolean to check if it closes
            bool IsComment = ReadFrom(Cursor, Cursor + 4) == U"<!--"; // boolean to check if its a comment
            bool IsMultiComment = ReadFrom(Cursor, Cursor + 4) == U"<!-~"; // boolean to check if its a multi comment

            if (IsComment)
            {
                MoveCursor(4); // consume the <!--
                while (InBounds() && ReadFrom(Cursor, Cursor + 4) != U"--!>") MoveCursor(1);
                // safety check if the user had closed the --!>
                if (!InBounds())
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : Comment is not closed";
                }
                MoveCursor(4); // consume the --!>
            
            } else if (IsMultiComment)
            {
                MoveCursor(4); // consume the <!-~
                while (InBounds() && ReadFrom(Cursor, Cursor + 4) != U"~-!>") MoveCursor(1);
                // safety check if the user had closed the ~-!>
                if (!InBounds())
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : Multi-Comment is not closed";
                }
                MoveCursor(4); // consume the ~-!>
            }
            else if (IsClose)
            {
                bool IsDummy = false;

                MoveCursor(2); // skip the </
                SkipSpaces(); // skips the space to see the scope name

                uint64_t ScopeNameCpB = Cursor;
                while (InBounds() && IsUEMLName(PeekCurChar())) MoveCursor(1);
                if (ReadFrom(ScopeNameCpB, Cursor).empty()) // its a dummy scope
                    IsDummy = true;
                    
                // write the close bytecode
                rglHexStringWriteAdvance(Output, (IsDummy ? std::vector<uint8_t>{0xFF, 0x09} : std::vector<uint8_t>{0xFF, 0x02}));

                if (!IsDummy) WriteString(ScopeNameCpB, Cursor - 1); // write it

                SkipSpaces(); // skip to check if the end exists

                if (PeekCurChar() != '>') // compile safety
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : At Scope end, expected > to close, got none/invalid " << PeekCurChar();
                }
                MoveCursor(1); // to move past the >
            } else // its a begin scope
            {
                bool IsSelfClosing = false;
                bool IsDummy = false;
                uint64_t MapCursor = 0;
                bool MapExists = false;
                bool IsbracketClosed = false; // flag to check if > existed

                // get the scope name
                MoveCursor(1);

                SkipSpaces(); // skip spaces first
                uint64_t ScopeNameCpB = Cursor;
                while (InBounds() && IsUEMLName(PeekCurChar())) MoveCursor(1);
                uint64_t ScopeNameCpE = Cursor;
                if (ReadFrom(ScopeNameCpB, Cursor).empty()) // its a dummy scope
                    IsDummy = true;

                SkipSpaces(); // skip to check if there exists a map properties

                if (PeekCurChar() == '[') // there exists a map properties
                {
                    MapCursor = Cursor;
                    MapExists = true;
                }

                // we need to skip the map first if it exists
                if (MapExists)
                {
                    // process bracket depth
                    int BracketDepth = 0;
                    bool InString = false;
                    char32_t StringChar = 0;
                    
                    while (InBounds())
                    {
                        char32_t c = PeekCurChar();
                        if (c == '\\')
                        {
                            MoveCursor(2);
                            continue;
                        }
                        
                        if (InString)
                        {
                            if (c == StringChar) InString = false;
                        }
                        else
                        {
                            if (c == '"' || c == '\'')
                            {
                                InString = true;
                                StringChar = c;
                            }
                            else if (c == '[')
                            {
                                BracketDepth++;
                            }
                            else if (c == ']')
                            {
                                BracketDepth--;
                                if (BracketDepth == 0)
                                {
                                    MoveCursor(1);
                                    break;
                                }
                            }
                        }
                        MoveCursor(1);
                    }
                    SkipSpaces(); // skip to get to the end part
                }

                // now check if its a self-closing scope to write
                if (ReadFrom(Cursor, Cursor + 2) == U"/>")
                {
                    IsSelfClosing = true;
                    IsbracketClosed = true;
                    MoveCursor(2); // consume />
                }
                else if (PeekCurChar() == '>') 
                {
                    IsbracketClosed = true; // safety checks
                    MoveCursor(1); // consume >
                }

                // write all of it
                uint64_t LastCursor = Cursor;
                rglHexStringWriteAdvance(Output, (
                    IsSelfClosing ?
                    (IsDummy ? std::vector<uint8_t>{0x00, 0x0B} : std::vector<uint8_t>{0x00, 0x0A})
                    :
                    (IsDummy ? std::vector<uint8_t>{0x00, 0x09} : std::vector<uint8_t>{0x00, 0x02})
                )); // write the bytecode depends on Self-closing & Dummy flags

                if (!IsDummy)
                    WriteString(ScopeNameCpB, ScopeNameCpE - 1);
                if (MapExists)
                {
                    // revert to Map cursor to read it
                    Cursor = MapCursor;
                    ReadValue(ValueType::Map, true);
                }
                
                Cursor = LastCursor;

                if (!IsbracketClosed)
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In scope opening, expected >, got none / invalid " << PeekCurChar();
                }
            }
        } else if (ReadFrom(Cursor, Cursor + 2) == U"[[") // if its an definition
        {
            rglHexStringWriteAdvance(Output, {0x00, 0x0C});
            MoveCursor(2); // skip the [[
            SkipSpaces(); // skip spaces
            if (PeekCurChar() != '$')
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : Non-variables cannot exist in double brackets";
                return;
            }
            uint64_t VariableNameCp = Cursor;
            MoveCursor(1); // move to avoid confusion of isUEMLname
            while (InBounds() && IsUEMLName(PeekCurChar())) MoveCursor(1); // advance until the end of the name + 1

            WriteString(VariableNameCp, Cursor - 1);
            SkipSpaces();
            if (PeekCurChar() == '=')
            {
                MoveCursor(1); // consume =
            } else
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In Variable definition brackets ([[]]), expected an assignment operator (=), got" <<
                    PeekCurChar();
                return;
            }

            SkipSpaces();
            ReadValue(ValueType::Any, false); // read recursive value

            SkipSpaces();
            if (ReadFrom(Cursor, Cursor + 2) != U"]]")
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLCompileError : In Variable definition brackets ([[]]), expected an closing operator (]]), got" <<
                    PeekCurChar();
                return;
            } else MoveCursor(2); // consume ]]
            rglHexStringWriteAdvance(Output, {0xFF, 0x0C});
        } else if (PeekCurChar() == '$') // if it was a variable reference
        {
            ReadValue(ValueType::VariableRef, false);
        }
        else // its body!
        {
            uint64_t BodyStringStart = Cursor;
            
            // read until hitting a variable marker ($), multi-bracket expression ([[), or unescaped tag open (<)
            while (InBounds())
            {
                if (PeekCurChar() == '\\')
                {
                    MoveCursor(2); // skip backlash
                    continue;
                }
                
                if (PeekCurChar() == '<' || PeekCurChar() == '$' || ReadFrom(Cursor, Cursor + 2) == U"[[")
                {
                    break;
                }
                
                MoveCursor(1);
            }
            
            // capture the raw multiline / intended block view
            std::u32string_view RBodView = ReadFrom(BodyStringStart, Cursor);
            
            // clean out the indentation gaps and newlines
            std::u32string ProBd = FormatString(RBodView);
            
            //  ! write if the body contains meaningful text after trimming
            if (!ProBd.empty())
            {
                rglHexStringWriteAdvance(Output, {0x00, 0x01}); // body open bytecode
                WriteStringDirectWO(ProBd);               
                rglHexStringWriteAdvance(Output, {0xFF, 0x01}); // body close byutecode
            }
        }
    }
}

inline void UEMLCompileSourceToFile(std::u32string Source, std::string BinaryPath)
{
    UEMLHexademicalFile hFile;
    if (!rglOpenHexFile(BinaryPath, &hFile)) return;

    UEMLHexademicalString temp;
    UEMLCompileSource(Source, &temp);

    std::vector<uint8_t> data(temp.Data.begin(), temp.Data.end());
    rglHexFileWriteAdvance(&hFile, data);
    
    rglCloseHexFile(&hFile);
}

#endif