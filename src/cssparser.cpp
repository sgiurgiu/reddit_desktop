#include "cssparser.h"
#include <algorithm>
#include <sstream>


namespace
{
constexpr auto CLASS_KEY = "class";
constexpr auto TAG_KEY = "tag";
constexpr auto ID_KEY = "id";
}
//This class does not properly collect the css properties
//as it ignores depth relation (among many other things) and some properties
//depending how the selectors are defined will probably end up being wrongly collected
//it's just enough for my needs here
//maybe one day we will revisit this
CSSParser::CSSParser(const std::string& str)
{
    KatanaOutput* katanaOut = katana_parse(str.c_str(), str.length(), KatanaParserModeStylesheet);
    auto stylesheet = katanaOut->stylesheet;

    for (size_t i = 0; i < stylesheet->rules.length; ++i)
    {
        KatanaRule* rule = static_cast<KatanaRule*>(stylesheet->rules.data[i]);
        if(rule->type == KatanaRuleStyle)
        {
           auto styleRule = (KatanaStyleRule*)(rule);
           SelectorsType ruleSelectors;
           for(size_t j=0; j< styleRule->selectors->length;j++)
           {
               KatanaSelector* selector = (KatanaSelector*)styleRule->selectors->data[j];
               ruleSelectors.merge(collectSelectorProperties(selector));
           }
           PropertiesType properties;
           for (size_t j = 0; j < styleRule->declarations->length; ++j)
           {
                KatanaDeclaration* decl = (KatanaDeclaration*)styleRule->declarations->data[j];
                if(decl->values)
                {
                    auto valuesVector = getValues(decl->values);
                    std::ranges::copy(valuesVector,
                                      std::back_inserter(properties[decl->property]));
                }
           }
           std::ranges::for_each(ruleSelectors,[&properties](auto&& selectors)
           {
               std::ranges::for_each(selectors.second,[&properties](auto&& selectorProp)
               {
                   selectorProp.second = properties;
               });
           });

           merge(ruleSelectors,selectors);
        }
    }

  //  katana_dump_output(katanaOut);

    katana_destroy_output(katanaOut);
}
std::optional<CSSParser::PropertiesType> CSSParser::getSelectorProperties(const std::string& key,
                                                                          const std::string& sel) const
{
    std::optional<PropertiesType> value;
    if(selectors.find(key) == selectors.end())
    {
        return value;
    }
    auto it = selectors.at(key).find(sel);
    if( it != selectors.at(key).end())
    {
        value = it->second;
    }
    return value;

}
std::optional<CSSParser::PropertiesType> CSSParser::getIdProperties(const std::string& id) const
{
    return getSelectorProperties(ID_KEY,id);
}
std::optional<CSSParser::PropertiesType> CSSParser::getClassProperties(const std::string& clazz) const
{
    return getSelectorProperties(CLASS_KEY,clazz);
}
std::optional<CSSParser::PropertiesType> CSSParser::getTagProperties(const std::string& tag) const
{
    return getSelectorProperties(TAG_KEY,tag);
}

std::string CSSParser::getValue(KatanaValue* value)
{
    std::string str;
    switch (value->unit) {
        case KATANA_VALUE_NUMBER:
        case KATANA_VALUE_PERCENTAGE:
        case KATANA_VALUE_EMS:
        case KATANA_VALUE_EXS:
        case KATANA_VALUE_REMS:
        case KATANA_VALUE_CHS:
        case KATANA_VALUE_PX:
        case KATANA_VALUE_CM:
        case KATANA_VALUE_DPPX:
        case KATANA_VALUE_DPI:
        case KATANA_VALUE_DPCM:
        case KATANA_VALUE_MM:
        case KATANA_VALUE_IN:
        case KATANA_VALUE_PT:
        case KATANA_VALUE_PC:
        case KATANA_VALUE_DEG:
        case KATANA_VALUE_RAD:
        case KATANA_VALUE_GRAD:
        case KATANA_VALUE_MS:
        case KATANA_VALUE_S:
        case KATANA_VALUE_HZ:
        case KATANA_VALUE_KHZ:
        case KATANA_VALUE_TURN:
            str = value->raw;
            break;
        case KATANA_VALUE_IDENT:
        case KATANA_VALUE_STRING:
            str = value->string;
            break;
        case KATANA_VALUE_PARSER_FUNCTION:
        {
            //not handled
            /*const char* args_str = katana_stringify_value_list(parser, value->function->args);
            snprintf(str, sizeof(str), "%s%s)", value->function->name, args_str);
            katana_parser_deallocate(parser, (void*) args_str);*/
            break;
        }
        case KATANA_VALUE_PARSER_OPERATOR:
            //not today
            if (value->iValue != '=')
            {
                //snprintf(str, sizeof(str), " %c ", value->iValue);
            } else
            {
                //snprintf(str, sizeof(str), " %c", value->iValue);
            }
            break;
        case KATANA_VALUE_PARSER_LIST:
            {
                auto valuesVector = getValues(value->list);
                std::ostringstream resultStream;
                std::ostream_iterator< std::string > oit( resultStream, " " );
                std::ranges::copy(valuesVector, oit );
                str = resultStream.str();
            }
            break;
        case KATANA_VALUE_PARSER_HEXCOLOR:
            str = "#";
            str += value->string;
            break;
        case KATANA_VALUE_URI:
            str = "url(";
            str += value->string;
            str += ")";
            break;
        default:
            break;
    }

    return str;
}
std::vector<std::string> CSSParser::getValues(KatanaArray* values)
{
    std::vector<std::string> strValues;
    for(size_t k=0;k<values->length;++k)
    {
        KatanaValue* value = (KatanaValue*)values->data[k];
        strValues.push_back(getValue(value));
    }
    return strValues;
}
CSSParser::SelectorsType CSSParser::collectSelectorProperties(KatanaSelector* selector)
{
    //we only go one level, the leaf. Why? Because we can.
    SelectorsType currentSelectors;
    switch(selector->match)
    {
    case KatanaSelectorMatchTag:
        currentSelectors[TAG_KEY].insert({selector->tag->local,{}});
        break;
    case KatanaSelectorMatchId:
        currentSelectors[ID_KEY].insert({selector->data->value,{}});
        break;
    case KatanaSelectorMatchClass:
        currentSelectors[CLASS_KEY].insert({selector->data->value,{}});
        break;
    default:
        break;
    }
    return currentSelectors;
}

void CSSParser::merge(SelectorsType& source,SelectorsType& destination)
{
    destination.merge(source);
    std::ranges::for_each(source,[&destination](auto&& sourceSelectorsPair)
    {
        destination[sourceSelectorsPair.first].merge(sourceSelectorsPair.second);
        auto&& destinationSelectors = destination[sourceSelectorsPair.first];
        std::ranges::for_each(sourceSelectorsPair.second,
                              [&destinationSelectors](auto&& selectorProp)
        {
            destinationSelectors[selectorProp.first].merge(selectorProp.second);
//            auto&& destinationProperties = destinationSelectors[selectorProp.first];
//            std::ranges::for_each(selectorProp.second,
//                                  [&destinationProperties](auto&& prop)
//            {
//                std::ranges::copy(prop.second,
//                                  std::back_inserter(destinationProperties[prop.first]));
//            });
        });
    });
}
