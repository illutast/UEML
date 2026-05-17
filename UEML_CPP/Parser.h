#ifndef UEMLPARSER_H
#define UEMLPARSER_H

#include "StringHex.h"
#include <boost/log/trivial.hpp>
#include <queue>
#include <optional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <fstream>

enum class UEMLNodeType
{
    Scope,
    Map,
    List,
    String,
    Number,
    Variable,
    Text,
    VariableReference
};

class UEMLNode
{
    public:
    UEMLNodeType NodeType;
    virtual ~UEMLNode() = default;
};

class UEMLMap : public UEMLNode
{
    public:
    std::unordered_map<std::u32string, std::shared_ptr<UEMLNode>> MapData;
};

class UEMLVariable : public UEMLNode
{
    public:
    std::u32string VariableName;
    std::shared_ptr<UEMLNode> VariableData;
};

class UEMLScope : public UEMLNode
{
    public:
    std::u32string ScopeName;
    std::vector<std::shared_ptr<UEMLNode>> Children;
    std::unordered_map<std::u32string, std::shared_ptr<UEMLVariable>> ScopeVariables;
    std::shared_ptr<UEMLMap> Properties = std::make_shared<UEMLMap>();
};

class UEMLNumber : public UEMLNode
{
    public:
    std::u32string Number;
};

class UEMLList : public UEMLNode
{
    public:
    std::vector<std::shared_ptr<UEMLNode>> ListData;
};

class UEMLString : public UEMLNode
{
    public:
    std::u32string Text;
};

class UEMLText : public UEMLNode
{
    public:
    std::u32string Text;
};

class UEMLVariableReference : public UEMLNode
{
    public:
    std::shared_ptr<UEMLVariable> Pointer;
};

class UEMLContentObject
{
    public: 
    std::vector<std::shared_ptr<UEMLNode>> GlobalContent;
    std::unordered_map<std::u32string, std::shared_ptr<UEMLVariable>> GlobalVariables;
};

// dont even think that this thing is nesscary

// literally converts u32->u8
inline std::string UEMLToUTF8(const std::u32string& U32Str)
{
    std::string UTF8Str;
    for (char32_t C : U32Str)
    {
        if (C <= 0x7F) {
            UTF8Str.push_back(static_cast<char>(C));
        } else if (C <= 0x7FF)
        {
            UTF8Str.push_back(static_cast<char>(0xC0 | ((C >> 6) & 0x1F)));
            UTF8Str.push_back(static_cast<char>(0x80 | (C & 0x3F)));
        } else if (C <= 0xFFFF)
        {
            UTF8Str.push_back(static_cast<char>(0xE0 | ((C >> 12) & 0x0F)));
            UTF8Str.push_back(static_cast<char>(0x80 | ((C >> 6) & 0x3F)));
            UTF8Str.push_back(static_cast<char>(0x80 | (C & 0x3F)));
        } else if (C <= 0x10FFFF)
        {
            UTF8Str.push_back(static_cast<char>(0xF0 | ((C >> 18) & 0x07)));
            UTF8Str.push_back(static_cast<char>(0x80 | ((C >> 12) & 0x3F)));
            UTF8Str.push_back(static_cast<char>(0x80 | ((C >> 6) & 0x3F)));
            UTF8Str.push_back(static_cast<char>(0x80 | (C & 0x3F)));
        }
    }
    return UTF8Str;
}


/// @brief Parses the compiled binary
inline void UEMLParseCompiledBinary(UEMLHexademicalString* String, UEMLContentObject* ContentObject)
{
    using namespace std::string_view_literals;
    
    uint64_t Cursor = 0; 
    std::deque<std::shared_ptr<UEMLScope>> NestingStack; 
    
    if (!String) {
        BOOST_LOG_TRIVIAL(error) << "UEMLparseerror : String input pointer is undefined (NULL)";
        return;
    }

    if (!ContentObject) {
        BOOST_LOG_TRIVIAL(error) << "UEMLparseerror : Content object pointer is undefined (NULL)";
        return;
    }

    // Check if cursor is in bounds
    auto CursorInBounds = [&]()
    {
        return Cursor < (uint64_t)String->Data.size();
    };

    // Read from A to B
    auto ReadFrom = [&](uint64_t A, uint64_t B)
    {
        if (B > String->Data.size()) B = String->Data.size();
        if (A >= B) return std::string_view();
        return std::string_view(String->Data.begin() + A, String->Data.begin() + B);
    };

    // Advances Steps*2 bytes
    auto MoveCursor = [&](uint64_t Steps = 1)
    {
        Cursor += Steps * 2;
    };
    
    auto PeekBytes = [&]()
    {
        return ReadFrom(Cursor, Cursor + 2);
    };

    auto CreateNode = [](UEMLNodeType Type) -> std::shared_ptr<UEMLNode>
    {
        switch (Type)
        {
            case UEMLNodeType::Scope:
            {
                auto Node = std::make_shared<UEMLScope>();
                Node->NodeType = Type;
                return Node;
            }
            case UEMLNodeType::Map:
            {
                auto Node = std::make_shared<UEMLMap>();
                Node->NodeType = Type;
                return Node;
            }
            case UEMLNodeType::List:
            {
                auto Node = std::make_shared<UEMLList>();
                Node->NodeType = Type;
                return Node;
            }
            case UEMLNodeType::String:
            {
                auto Node = std::make_shared<UEMLString>();
                Node->NodeType = Type;
                return Node;
            }
            case UEMLNodeType::Number:
            {
                auto Node = std::make_shared<UEMLNumber>();
                Node->NodeType = Type;
                return Node;
            }
            case UEMLNodeType::Variable:
            {
                auto Node = std::make_shared<UEMLVariable>();
                Node->NodeType = Type;
                return Node;
            }
            case UEMLNodeType::Text:
            {
                auto Node = std::make_shared<UEMLText>();
                Node->NodeType = Type;
                return Node;
            }
            case UEMLNodeType::VariableReference:
            {
                auto Node = std::make_shared<UEMLVariableReference>();
                Node->NodeType = Type;
                return Node;
            }
            default:
                return nullptr;
        }
    };

    // parses 4-byte BOD_C values
    auto ParseUEMLString = [&](uint64_t S, uint64_t E) -> std::u32string
    {
        std::u32string Res = U"";

        for (uint64_t i = S; i < E; i += 8)
        {
            if (i + 6 >= String->Data.size()) break;

            uint32_t Codepoint = 0;
            Codepoint |= (static_cast<uint32_t>(static_cast<unsigned char>(String->Data[i])) << 24);
            Codepoint |= (static_cast<uint32_t>(static_cast<unsigned char>(String->Data[i + 2])) << 16);
            Codepoint |= (static_cast<uint32_t>(static_cast<unsigned char>(String->Data[i + 4])) << 8);
            Codepoint |= static_cast<uint32_t>(static_cast<unsigned char>(String->Data[i + 6]));

            Res.push_back(static_cast<char32_t>(Codepoint));
        }
        return Res;
    };

    // Reads the value and returns the designated node.
    std::function<std::shared_ptr<UEMLNode>(bool)> ReadValue = [&](bool IsProperties) -> std::shared_ptr<UEMLNode>
    {
        auto Header = PeekBytes();

        // is a number
        if (Header == "\x00\x04"sv)
        {
            auto Node = std::static_pointer_cast<UEMLNumber>(CreateNode(UEMLNodeType::Number));
            MoveCursor(); // move past the number bytecode
            uint64_t NumberCp = Cursor;
            while (CursorInBounds() && PeekBytes() != "\xFF\x04"sv) MoveCursor(); // move until the end of number (0xFF0x04)
            Node->Number = ParseUEMLString(NumberCp, Cursor);
            MoveCursor();
            return Node;
        }
        // is a string
        else if (Header == "\x00\x03"sv)
        {
            auto Node = std::static_pointer_cast<UEMLString>(CreateNode(UEMLNodeType::String));
            MoveCursor(); // move past the string bytecode
            uint64_t StringCp = Cursor;
            while (CursorInBounds() && PeekBytes() != "\xFF\x03"sv) MoveCursor(); // move until the end of string (0xFF0x03)
            Node->Text = ParseUEMLString(StringCp, Cursor);
            MoveCursor();
            return Node;
        }
        // is a list
        else if (Header == "\x00\x05"sv)
        {
            auto Node = std::static_pointer_cast<UEMLList>(CreateNode(UEMLNodeType::List));
            MoveCursor(); // get past that list bytecode
            while (CursorInBounds() && PeekBytes() != "\xFF\x05"sv) // loop until the end of list
            {
                auto ENode = ReadValue(false); // recursive reading
                if (ENode)
                {
                    Node->ListData.push_back(ENode);
                } else
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : Invalid data type in List object";
                    return nullptr;
                }  
                // because of the order, the while loop automatically detects if the list ends, so skip here
            };
            MoveCursor();
            return Node;
        }
        // is a map
        else if (Header == "\x00\x06"sv)
        {
            auto Node = std::static_pointer_cast<UEMLMap>(CreateNode(UEMLNodeType::Map));
            MoveCursor(); //get past that map bytecode
            while (CursorInBounds() && PeekBytes() != "\xFF\x06"sv) // loop until the end of map
            {
                MoveCursor(); // move past the string begin bytecode
                uint64_t KeyCp = Cursor;
                while (CursorInBounds() && PeekBytes() != "\xFF\x03"sv) MoveCursor();
                auto Key = ParseUEMLString(KeyCp, Cursor);  
                if (!Key.empty() && Key[0] == '$' && IsProperties) // Its a variable definition
                {
                    auto Node = std::static_pointer_cast<UEMLVariable>(CreateNode(UEMLNodeType::Variable));
                    Node->VariableName = Key;
                    NestingStack.back()->ScopeVariables[Node->VariableName] = Node;
                    MoveCursor(); // move past the string end bytecode
                    if (auto Obj = ReadValue(false))
                    {
                        Node->VariableData = Obj;
                    } else
                    {
                        BOOST_LOG_TRIVIAL(error) << "UEMLParseError : Invalid data (type) in Variable definition of Scope properties";
                        return nullptr;
                    }
                    continue; // Continue because variable should not be saved into properties
                } else if (!Key.empty() && Key[0] == '$')
                { // we knew that in Compiler.h we had addressed the part that variable def is not allowed but for safety here goes
                    BOOST_LOG_TRIVIAL(error) << "UEMLParseError : Variable definition is not allowed in normal maps";
                    return nullptr;
                }
                MoveCursor(); // move past the string end bytecode
                if (auto Obj = ReadValue(false))
                {
                    Node->MapData[Key] = Obj;
                } else
                {
                    BOOST_LOG_TRIVIAL(error) << "UEMLParseError : Invalid data (type) in a key definition (Map)";
                    return nullptr;
                }
                // because of the order, the while loop automatically detects if the map ends, so skip here
            }
            MoveCursor(); // moves the cursor to the next object (maybe)
            return Node;
        }
        // is a Variable Reference
        else if (Header == "\x00\x07"sv)
        {
            auto Node = std::static_pointer_cast<UEMLVariableReference>(CreateNode(UEMLNodeType::VariableReference));
            MoveCursor(); //get past that VR bytecode 
            uint64_t VRCp = Cursor;
            while (CursorInBounds() && PeekBytes() != "\xFF\x07"sv) MoveCursor(); // advance until end of VR end bytecode
            auto VariableName = ParseUEMLString(VRCp, Cursor); // read var name
            MoveCursor(); // move past the end of varaible
            // check for existence
            if (NestingStack.empty())
            {
                auto Itr = ContentObject->GlobalVariables.find(VariableName);
                if (Itr == ContentObject->GlobalVariables.end())
                {
                    BOOST_LOG_TRIVIAL(error) << "UEMLParseError : Invalid variable reference. Check if the variable exists?";
                    return nullptr;
                } else
                {
                    Node->Pointer = Itr->second;
                    return Node;
                }
            } else
            {
                std::deque Temp = NestingStack;
                while (!Temp.empty())
                {
                    auto Itr = Temp.back()->ScopeVariables.find(VariableName);
                    if (Itr == Temp.back()->ScopeVariables.end())
                    {
                        Temp.pop_back();
                    } else
                    {
                        Node->Pointer = Itr->second;
                        return Node;
                    }
                }
                auto Itr = ContentObject->GlobalVariables.find(VariableName);
                if (Itr == ContentObject->GlobalVariables.end())
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : Invalid variable reference. Check if the variable exists?";
                    return nullptr;
                } else
                {
                    Node->Pointer = Itr->second;
                    return Node;
                }
            }
            return Node;
        }
        return nullptr;
    };

    // Begin file

    if (ReadFrom(0, 4) != "UEML")
    {
        BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : Invalid file identity signature (or simpler, this file is not generated by UEML compiler!)";
        return;
    }
    MoveCursor(4); // move past the ascii UEML

    // Parse scopes
    while (CursorInBounds()) // read until end of binary file
    {
        auto Header = PeekBytes();

        if (Header == "\x00\x02"sv) // normal scope open
        {
            auto Node = static_pointer_cast<UEMLScope>(CreateNode(UEMLNodeType::Scope));

            MoveCursor(2); // move past that scope open bytecode & string open bytecode
            uint64_t ScopeNamePos = Cursor;
            while (CursorInBounds() && PeekBytes() != "\xFF\x03"sv) MoveCursor(); // move until meet string end bytecode
            auto Name = ParseUEMLString(ScopeNamePos, Cursor);
            Node->ScopeName = Name;
            MoveCursor(); // move to check if map exists or it is body

            NestingStack.push_back(Node);

            // check if ScopeProperties exists
            if (PeekBytes() == "\x00\x06"sv)
            {
                auto Map = ReadValue(true); //if so, read it
                if (!Map || Map->NodeType != UEMLNodeType::Map) // check if it is not the Map
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : In Scope, Expected properties to be a Map, got invalid";
                    return;
                }
                Node->Properties = static_pointer_cast<UEMLMap>(Map); // goes into properties
            }
            
            if (NestingStack.size() == 1)
                ContentObject->GlobalContent.push_back(Node);
            else
                NestingStack[NestingStack.size() - 2]->Children.push_back(Node);
            // because we had MoveCursor and Readvalue for each case, the cursor is already outside the normal scope so we dont have to do anything
        } else if (Header == "\xFF\x02"sv) // normal scope close
        {
            MoveCursor(2); // move past that scope end bytecode  & string open bytecode
            uint64_t ScopeNamePos = Cursor;
            while (CursorInBounds() && PeekBytes() != "\xFF\x03"sv) MoveCursor(); // move until meet string end bytecode
            auto Name = ParseUEMLString(ScopeNamePos, Cursor);
            MoveCursor(); // step through string end bytecode
            if (NestingStack.empty())
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : No scope named " << UEMLToUTF8(Name) << " exists to close";
                return;
            } else if (NestingStack.back()->ScopeName != Name)
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : Scope " << UEMLToUTF8(NestingStack.back()->ScopeName) <<
                                        " was expected to be closed first, but got" << UEMLToUTF8(Name);
            } else
            {
                NestingStack.pop_back(); // we are safe to pop back that scope because it exits
            }
        } else if (Header == "\x00\x09"sv) // Dummy scope open
        {
            auto Node = static_pointer_cast<UEMLScope>(CreateNode(UEMLNodeType::Scope));
            Node->ScopeName = U""; // Because it is a dummy scope, it has no name
            MoveCursor(); // move past the dummy scope open bytecode

            NestingStack.push_back(Node);
            
            // check if ScopeProperties exists
            if (PeekBytes() == "\x00\x06"sv)
            {
                auto Map = ReadValue(true); //if so, read it
                if (!Map || Map->NodeType != UEMLNodeType::Map) // check if it is not the Map
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : In Scope, Expected properties to be a Map, got invalid";
                    return;
                }
                Node->Properties = static_pointer_cast<UEMLMap>(Map); // goes into properties
            }
            // because we had MoveCursor and Readvalue for each case, the cursor is already outside the normal scope so we dont have to do anything
            if (NestingStack.size() == 1)
                ContentObject->GlobalContent.push_back(Node);
            else
                NestingStack[NestingStack.size() - 2]->Children.push_back(Node);
        } else if (Header == "\xFF\x09"sv) // Dummy scope close
        {
            MoveCursor(1); // move past the dummy scope close
            if (NestingStack.empty())
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : No dummy scope / scope exists to close dummy scope";
                return;
            } else if (NestingStack.back()->ScopeName != U"")
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : Scope " << UEMLToUTF8(NestingStack.back()->ScopeName) << 
                                            "was expected to close first, got dummy scope";
                return;
            } else
            {
                NestingStack.pop_back();
            }
        } else if (Header == "\x00\x0A"sv) // self closing scope
        {
            auto Node = static_pointer_cast<UEMLScope>(CreateNode(UEMLNodeType::Scope));
            MoveCursor(2); // move past the self-closing scope bytecode and string bytecode

            uint64_t ScopeNamePos = Cursor;
            while (CursorInBounds() && PeekBytes() != "\xFF\x03"sv) MoveCursor(); // move until meet string end bytecode
            Node->ScopeName = ParseUEMLString(ScopeNamePos, Cursor);

            MoveCursor(); // see if it has properties

            if (CursorInBounds() && PeekBytes() == "\x00\x06"sv)
            {
                auto Map = ReadValue(true); //if so, read it
                if (!Map || Map->NodeType != UEMLNodeType::Map) // check if it is not the Map
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : In Scope, Expected properties to be a Map, got invalid";
                    return;
                }
                Node->Properties = static_pointer_cast<UEMLMap>(Map); // goes into properties
            }

            // even if it is self-closing we still need to register it to the tree
            if (NestingStack.empty())
            { // the scope is the first entry in Contentobject
                ContentObject->GlobalContent.push_back(Node);
            } else
            { // means the scope belongs to another scope
                NestingStack.back()->Children.push_back(Node);
            }
        } else if (Header == "\x00\x0B"sv) // self closing dummy scope
        {
            auto Node = static_pointer_cast<UEMLScope>(CreateNode(UEMLNodeType::Scope));
            MoveCursor(1); // move past the self-closing scope bytecode

            Node->ScopeName = U""; // its a dumy scope

            if (CursorInBounds() && PeekBytes() == "\x00\x06"sv)
            {
                auto Map = ReadValue(true); //if so, read it
                if (!Map || Map->NodeType != UEMLNodeType::Map) // check if it is not the Map
                {
                    BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : In Scope, Expected properties to be a Map, got invalid";
                    return;
                }
                Node->Properties = static_pointer_cast<UEMLMap>(Map); // goes into properties
            }

            // even if it is self-closing we still need to register it to the tree
            if (NestingStack.empty())
            { // the scope is the first entry in Contentobject
                ContentObject->GlobalContent.push_back(Node);
            } else
            { // means the scope belongs to another scope
                NestingStack.back()->Children.push_back(Node);
            }
        } else if (Header == "\x00\x0C"sv) // variable definition block
        {
            auto Node = static_pointer_cast<UEMLVariable>(CreateNode(UEMLNodeType::Variable));
            MoveCursor(2); // move past the VD bytecode and the string bytecode
            uint64_t VariableNameCp = Cursor;
            while (CursorInBounds() && PeekBytes() != "\xFF\x03"sv) MoveCursor();
            Node->VariableName = ParseUEMLString(VariableNameCp, Cursor);
            MoveCursor(); // move outside the end of the string to read data
            auto VarData = ReadValue(false);
            if (!VarData)
            {
                BOOST_LOG_TRIVIAL(fatal) << "UEMLParseError : Invalid value in variable definition";
                return;
            }
            Node->VariableData = VarData;
            MoveCursor(); // consume VD endbyte marker;
            if (!NestingStack.empty())
                NestingStack.back()->ScopeVariables[Node->VariableName] = Node;
            else
                ContentObject->GlobalVariables[Node->VariableName] = Node;
        } else if (Header == "\x00\x07"sv) // Standalone Variable Reference
        {
            auto Node = ReadValue(false); // ReadValue already handles 0x00 0x07 logic
            if (Node)
            {
                if (NestingStack.empty())
                    ContentObject->GlobalContent.push_back(Node);
                else
                    NestingStack.back()->Children.push_back(Node);
            }
        }
        else if (Header == "\x00\x01"sv) // its body open bytecode
        {
            auto Node = std::static_pointer_cast<UEMLText>(CreateNode(UEMLNodeType::Text));
            MoveCursor(); // move past body open bytecode

            uint64_t TextCp = Cursor; // starts exactly at the first char
            
            // advance till meet the end of body bytecode
            while (CursorInBounds() && PeekBytes() != "\xFF\x01"sv) 
            {
                MoveCursor(); 
            }
            
            // parse
            Node->Text = ParseUEMLString(TextCp, Cursor);

            if (CursorInBounds() && PeekBytes() == "\xFF\x01"sv) 
            {
                MoveCursor();
            }

            if (NestingStack.empty()) ContentObject->GlobalContent.push_back(Node);
            else NestingStack.back()->Children.push_back(Node);
        }
    }

}   
inline std::u32string UEMLUTF8ToU32(const std::string& UTF8str)
{
    std::u32string Utf32;
    Utf32.reserve(UTF8str.size());

    for (size_t i = 0; i < UTF8str.size(); )
    {
        uint32_t Cp = 0;
        unsigned char Cr = static_cast<unsigned char>(UTF8str[i]);

        if (Cr <= 0x7F)
        {
            Cp = Cr;
            i += 1;
        }
        else if ((Cr & 0xE0) == 0xC0)
        {
            Cp = (Cr & 0x1F) << 6;
            Cp |= (static_cast<unsigned char>(UTF8str[i + 1]) & 0x3F);
            i += 2;
        }
        else if ((Cr & 0xF0) == 0xE0)
        {
            Cp = (Cr & 0x0F) << 12;
            Cp |= (static_cast<unsigned char>(UTF8str[i + 1]) & 0x3F) << 6;
            Cp |= (static_cast<unsigned char>(UTF8str[i + 2]) & 0x3F);
            i += 3;
        }
        else if ((Cr & 0xF8) == 0xF0)
        {
            Cp = (Cr & 0x07) << 18;
            Cp |= (static_cast<unsigned char>(UTF8str[i + 1]) & 0x3F) << 12;
            Cp |= (static_cast<unsigned char>(UTF8str[i + 2]) & 0x3F) << 6;
            Cp |= (static_cast<unsigned char>(UTF8str[i + 3]) & 0x3F);
            i += 4;
        }
        else
        {
            i++;
            continue;
        }
        Utf32.push_back(static_cast<char32_t>(Cp));
    }
    return Utf32;
}

inline std::u32string UEMLReadSourceFile(std::string FilePath) {
    std::ifstream inFile(FilePath, std::ios::binary);
    if (!inFile)
    {
        BOOST_LOG_TRIVIAL(error) << "UEMLparsecheck: Could not open " << FilePath;
        return U"";
    }

    std::string utf8((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

    return UEMLUTF8ToU32(utf8);
}

#endif //UEMLPARSER_H