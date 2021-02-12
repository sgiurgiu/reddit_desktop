#ifndef CSSPARSER_H
#define CSSPARSER_H

#include <string>
#include <unordered_map>
#include <vector>

#include <katana.h>

class CSSParser
{
public:
    using ValuesType = std::vector<std::string>;
    using PropertiesType  = std::unordered_map<std::string,ValuesType>;
    CSSParser(const std::string& str);
    std::optional<PropertiesType> getIdProperties(const std::string& id) const;
    std::optional<PropertiesType> getClassProperties(const std::string& clazz) const;
    std::optional<PropertiesType> getTagProperties(const std::string& tag) const;
private:
    using SelectorPropertiesType = std::unordered_map<std::string,PropertiesType>;
    using SelectorsType = std::unordered_map<std::string,SelectorPropertiesType>;
    SelectorsType collectSelectorProperties(KatanaSelector* selector);
    void merge(SelectorsType& source,SelectorsType& destination);
    std::string getValue(KatanaValue* value);
    std::vector<std::string> getValues(KatanaArray* values);
    std::optional<PropertiesType> getSelectorProperties(const std::string& key,const std::string& sel) const;
private:
    SelectorsType selectors;
};

#endif // CSSPARSER_H
