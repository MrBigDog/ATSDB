#include <boost/assign/list_of.hpp>

#include "structureelement.h"

std::map<StructureElementDataType,std::string> StructureElement::data_type_strings_ = boost::assign::map_list_of
        (StructureElementDataType::BOOL,          "bool")
        (StructureElementDataType::TINYINT,       "tinyint")
        (StructureElementDataType::UTINYINT,      "utinyint")
        (StructureElementDataType::SMALLINT,      "smallint")
        (StructureElementDataType::USMALLINT,     "usmallint")
        (StructureElementDataType::INT,           "int")
        (StructureElementDataType::UINT,          "uint")
        (StructureElementDataType::VARCHAR,       "varchar")
        (StructureElementDataType::VARCHAR_ARRAY, "varchar_array")
        (StructureElementDataType::FLOAT,         "float")
        (StructureElementDataType::DOUBLE,        "double");

std::map<StructureElementDataType, size_t> StructureElement::data_type_sizes_ = boost::assign::map_list_of
        (StructureElementDataType::BOOL,          sizeof(bool))
        (StructureElementDataType::TINYINT,       sizeof(char))
        (StructureElementDataType::UTINYINT,      sizeof(unsigned char))
        (StructureElementDataType::SMALLINT,      sizeof(short int))
        (StructureElementDataType::USMALLINT,     sizeof(unsigned short int))
        (StructureElementDataType::INT,           sizeof(int))
        (StructureElementDataType::UINT,          sizeof(unsigned int))
        (StructureElementDataType::VARCHAR,       sizeof(unsigned char))
        (StructureElementDataType::VARCHAR_ARRAY, sizeof(unsigned char))
        (StructureElementDataType::FLOAT,         sizeof(float))
        (StructureElementDataType::DOUBLE,        sizeof(float));

std::map<StructureElementDataType,PropertyDataType> StructureElement::data_type_conversion_table_ = boost::assign::map_list_of
        (StructureElementDataType::BOOL,          PropertyDataType::BOOL)
        (StructureElementDataType::TINYINT,       PropertyDataType::CHAR)
        (StructureElementDataType::UTINYINT,      PropertyDataType::UCHAR)
        (StructureElementDataType::SMALLINT,      PropertyDataType::INT)
        (StructureElementDataType::USMALLINT,     PropertyDataType::UINT)
        (StructureElementDataType::INT,           PropertyDataType::INT)
        (StructureElementDataType::UINT,          PropertyDataType::UINT)
        (StructureElementDataType::VARCHAR,       PropertyDataType::STRING)
        (StructureElementDataType::VARCHAR_ARRAY, PropertyDataType::STRING)
        (StructureElementDataType::FLOAT,         PropertyDataType::FLOAT)
        (StructureElementDataType::DOUBLE,        PropertyDataType::DOUBLE);

StructureElement::StructureElement()
    : present_variable_ (0)
{

}