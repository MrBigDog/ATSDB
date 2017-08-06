/*
 * This file is part of ATSDB.
 *
 * ATSDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ATSDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with ATSDB.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * DBCondition.cpp
 *
 *  Created on: Nov 6, 2011
 *      Author: sk
 */

#include <sstream>
#include <cassert>

#include <QHBoxLayout>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>

#include <boost/algorithm/string/join.hpp>

#include "dbfiltercondition.h"
#include "dbobjectmanager.h"
#include "dbobject.h"
#include "dbovariable.h"
#include "metadbovariable.h"
#include "dbschemamanager.h"
#include "metadbtable.h"
#include "dbschema.h"
#include "dbtable.h"
#include "dbtablecolumn.h"
#include "atsdb.h"
#include "dbfilter.h"
#include "unitmanager.h"
#include "unit.h"

#include "stringconv.h"

using namespace Utils;

/**
 * Initializes members, registers parameters, create GUI elements.
 */
DBFilterCondition::DBFilterCondition(const std::string &class_id, const std::string &instance_id, DBFilter *filter_parent)
: Configurable (class_id, instance_id, filter_parent), filter_parent_(filter_parent), variable_(nullptr), meta_variable_(nullptr), changed_(true)
{
    registerParameter ("operator", &operator_, ">");
    registerParameter ("op_and", &op_and_, true);
    registerParameter ("absolute_value", &absolute_value_, false);
    registerParameter ("variable_dbo_name", &variable_dbo_name_, "");
    registerParameter ("variable_name", &variable_name_, "");

    if (variable_dbo_name_ == META_OBJECT_NAME)
    {
        if (!ATSDB::instance().objectManager().existsMetaVariable(variable_name_))
            throw std::runtime_error ("DBFilterCondition: constructor: meta dbo variable '"+variable_name_+"' does not exist");
        meta_variable_ = &ATSDB::instance().objectManager().metaVariable(variable_name_);
    }
    else
    {
        if (!ATSDB::instance().objectManager().existsObject(variable_dbo_name_) || !ATSDB::instance().objectManager().object(variable_dbo_name_).hasVariable(variable_name_))
            throw std::runtime_error ("DBFilterCondition: constructor: dbo variable '"+variable_name_+"' does not exist");

        variable_ = &ATSDB::instance().objectManager().object(variable_dbo_name_).variable(variable_name_);
    }

    // TODO ADD THIS LATER
    //variable_->addMinMaxObserver(this);

    registerParameter ("reset_value", &reset_value_, std::string("value"));
    registerParameter ("value", &value_, "");

    widget_ = new QWidget ();
    QHBoxLayout *layout = new QHBoxLayout ();
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (0);

    label_ = new QLabel(tr((variable_name_+" "+operator_).c_str()));
    layout->addWidget(label_);

    edit_ = new QLineEdit(tr(value_.c_str()));
    connect(edit_, SIGNAL( returnPressed() ), this, SLOT( valueChanged() ));
    layout->addWidget(edit_);

    widget_->setLayout (layout);
}

DBFilterCondition::~DBFilterCondition()
{
    //TODO ADD THIS LATER
    //variable_->removeMinMaxObserver(this);
}

void DBFilterCondition::invert ()
{
    //TODO
    //op_and_=!op_and_;
}

/**
 * Returns if variable_ exists in DBObject of type
 */
bool DBFilterCondition::filters (const std::string &dbo_name)
{
    if (meta_variable_)
    {
        return meta_variable_->existsIn(dbo_name);
    }
    else
        return variable_dbo_name_ == dbo_name;
}

std::string DBFilterCondition::getConditionString (const std::string &dbo_name, bool &first, std::vector<std::string> &variable_names)
{
    logdbg << "DBFilterCondition: getConditionString: object " << dbo_name << " first " << first;
    std::stringstream ss;

    std::string variable_prefix;
    std::string variable_suffix;

    if (absolute_value_)
    {
        variable_prefix="ABS("+variable_prefix;
        variable_suffix=variable_suffix+")";
    }

    assert (variable_ || meta_variable_);

    //DBSchemaManager &schema_manager = ATSDB::instance().schemaManager();

    DBOVariable *variable=nullptr;

    if (meta_variable_)
    {
        assert (meta_variable_->existsIn(dbo_name));
        variable = &meta_variable_->getFor(dbo_name);
    }
    else
        variable = variable_;

    const DBTableColumn &column = variable->currentDBColumn();
    const MetaDBTable &meta_table = variable->currentMetaTable();
    std::string table_db_name = meta_table.tableFor(column.name()).name();

    if (!first)
    {
        if (op_and_)
            ss << " AND ";
        else
            ss << " OR ";
    }
    first=false;

    std::vector<std::string> value_strings;
    std::vector<std::string> transformed_value_strings;

    if (operator_ == "IN")
    {
        value_strings = String::split(value_, ',');
    }
    else
    {
        value_strings.push_back(value_);
    }

    loginf << "DBFilterCondition: getConditionString: in value strings '" << boost::algorithm::join(value_strings, ",") << "'";

    for (auto value_it : value_strings)
    {
        std::string value_str = value_it;

        if (variable->representation() != String::Representation::STANDARD)
            value_str = String::getValueStringFromRepresentation(value_str, variable->representation()); // fix representation

        loginf << "DBFilterCondition: getConditionString: value string " << value_str;

        if (column.unit() != variable->unit()) // do unit conversion stuff
        {
            logdbg << "DBFilterCondition: getConditionString: variable " << variable->name() << " of same dimension has different units " << column.unit() << " " << variable->unit();

            const Dimension &dimension = UnitManager::instance().dimension (variable->dimension());
            double factor = dimension.getFactor (column.unit(), variable->unit());
            logdbg  << "DBFilterCondition: getConditionString: correct unit transformation with factor " << factor;

            switch (variable->dataType())
            {
            case PropertyDataType::BOOL:
            case PropertyDataType::UCHAR:
            case PropertyDataType::UINT:
            case PropertyDataType::ULONGINT:
            {
                unsigned long value = std::stoul(value_str);
                value /= factor;
                value_str = std::to_string(value);
                break;
            }
            case PropertyDataType::CHAR:
            case PropertyDataType::INT:
            case PropertyDataType::LONGINT:
            {
                long value = std::stol(value_str);
                value /= factor;
                value_str = std::to_string(value);
                break;
            }
            case PropertyDataType::FLOAT:
            case PropertyDataType::DOUBLE:
            {
                double value = std::stod(value_str);
                value /= factor;
                value_str = Utils::String::getValueString(value);
                break;
            }
            case PropertyDataType::STRING:
                logerr << "DBFilterCondition:getConditionString: unit transformation for string variable " << variable->name() << " impossible";
                break;
            default:
                logerr  <<  "DBFilterCondition:getConditionString: unknown property type " << Property::asString(variable->dataType());
                throw std::runtime_error ("DBFilterCondition:getConditionString: unknown property type "+Property::asString(variable->dataType()));
            }
        }
        loginf << "DBFilterCondition: getConditionString: transformed value string " << value_str;
        transformed_value_strings.push_back(value_str);
    }

    assert (transformed_value_strings.size());

    if (operator_ != "IN")
    {
        assert (transformed_value_strings.size() == 1);
        ss << table_db_name << "." << column.name() << " " << operator_ << " " << transformed_value_strings.at(0);
    }
    else
        ss << table_db_name << "." << column.name() << " " << operator_ << " (" << boost::algorithm::join(transformed_value_strings, ",") << ")";

    if (find (variable_names.begin(), variable_names.end(), variable->name()) == variable_names.end())
        variable_names.push_back(variable->name());


    if (ss.str().size() > 0)
        logdbg  << "DBFilterCondition " << instance_id_<<": getConditionString: '" << ss.str()<<"'";

    return ss.str();

        //TODO FIX REPRESENATION
        //                ss << variable_prefix << table_db_name << "." << variable_->id_  << variable_suffix  << " " << operator_ << " "
        //                        << variable_->getValueFromRepresentation(value_, true);

//    if (operator_.compare("|=") == 0)
//    {
//        std::vector<std::string> chunks = String::split(value_, ',');

//        if (chunks.size() > 0)
//        {
//            //loginf  << "only subs";

//            // TODO why
////            MetaDBTable &meta_table_name = variable_->currentMetaTable();
////            std::string table_db_name = meta_table_name.t(variable_->getId());
//            //ss << table_db_name << "." << variable_->id_;

//            if (variable_->dboName() == dbo_type)
//            {
//                if (!first)
//                {
//                    if (op_and_)
//                        ss << " AND ";
//                    else
//                        ss << " OR ";

//                    first=false;
//                }

//                ss  << "(";

//                for (unsigned int cnt2 = 0; cnt2 < chunks.size(); cnt2++)
//                {
//                    if (cnt2 != 0)
//                        ss << " OR ";

//                    assert (false);
//                    // FIX representation
//                    //                        ss << variable_prefix << table_db_name << "." << variable_->getId()  << variable_suffix << " = "
//                    //                                << variable_->getValueFromRepresentation(chunks.at(cnt2), true);
//                }
//                ss  << ")";

//                if (chunks.size() > 0) // variable added
//                {
//                    if (find (variable_names.begin(), variable_names.end(), variable_->name()) == variable_names.end())
//                        variable_names.push_back(variable_->name());
//                }
//            }
            //            else // is meta
//            {
//                const std::map <std::string, std::string> &subvars = variable_->getSubVariables ();
//                std::map <std::string, std::string>::const_iterator it;

//                for (it =subvars.begin(); it != subvars.end(); it++)
//                {
//                    DBOVariable *tmpvar = variable_->getFor (it->first);

//                    std::string meta_table_name = tmpvar->getCurrentMetaTable();
//                    assert (DBSchemaManager::getInstance().getCurrentSchema()->hasMetaTable(meta_table_name));
//                    MetaDBTable *table =DBSchemaManager::getInstance().getCurrentSchema()->getMetaTable(meta_table_name);
//                    std::string table_db_name = table->getTableDBNameForVariable(tmpvar->getId());
//                    //ss << table_db_name << "." << variable_->id_;


//                    if (tmpvar->getDBOType() == dbo_type)
//                    {
//                        if (!first)
//                        {
//                            if (op_and_)
//                                ss << " AND ";
//                            else
//                                ss << " OR ";

//                            first=false;
//                        }

//                        ss  << "(";

//                        for (unsigned int cnt2 = 0; cnt2 < chunks.size(); cnt2++)
//                        {
//                            if (cnt2 != 0)
//                                ss << " OR ";

//                            //TODO FIX REPRESENTATION
//                            assert (false);
///*                            ss << variable_prefix << table_db_name<<"." <<tmpvar->id_  << variable_suffix << " = "
//                                    << variable_->getValueFromRepresentation(chunks.at(cnt2), true)*/;
//                        }
//                        ss  << ")";

//                        if (chunks.size() > 0) // variable added
//                        {
//                            if (find (variable_names.begin(), variable_names.end(), tmpvar->getId()) == variable_names.end())
//                                variable_names.push_back(tmpvar->getId());
//                        }

//                    }
//                }
//            }
//        }
//    }
//    else
//    {
//        //TODO why?
////            std::string meta_table_name = variable_->getCurrentMetaTable();
////            assert (DBSchemaManager::getInstance().getCurrentSchema()->hasMetaTable(meta_table_name));
////            MetaDBTable *table =DBSchemaManager::getInstance().getCurrentSchema()->getMetaTable(meta_table_name);
////            std::string table_db_name = table->getTableDBNameForVariable(variable_->getId());
//            //ss << table_db_name << "." << variable_->id_;

//            if (variable_->dboName() == dbo_type)
//            {
//                if (!first)
//                {
//                    if (op_and_)
//                        ss << " AND ";
//                    else
//                        ss << " OR ";

//                    first=false;
//                }

//                //TODO FIX REPRESENATION
//                assert (false);
////                ss << variable_prefix << table_db_name << "." << variable_->id_  << variable_suffix  << " " << operator_ << " "
////                        << variable_->getValueFromRepresentation(value_, true);

//                if (find (variable_names.begin(), variable_names.end(), variable_->name()) == variable_names.end())
//                    variable_names.push_back(variable_->name());
//            }

//        else // is meta
//        {
//            const std::map <std::string, std::string> &subvars = variable_->getSubVariables ();
//            std::map <std::string, std::string>::const_iterator it;

//            for (it =subvars.begin(); it != subvars.end(); it++)
//            {
//                DBOVariable *tmpvar = variable_->getFor (it->first);

//                std::string meta_table_name = tmpvar->getCurrentMetaTable();
//                assert (DBSchemaManager::getInstance().getCurrentSchema()->hasMetaTable(meta_table_name));
//                MetaDBTable *table =DBSchemaManager::getInstance().getCurrentSchema()->getMetaTable(meta_table_name);
//                std::string table_db_name = table->getTableDBNameForVariable(tmpvar->getId());
//                //ss << table_db_name << "." << variable_->id_;

//                if (tmpvar->getDBOType() == dbo_type)
//                {
//                    if (!first)
//                    {
//                        if (op_and_)
//                            ss << " AND ";
//                        else
//                            ss << " OR ";

//                        first=false;
//                    }
//                    //TODO FIX REPRESENTATION
//                    assert (false);
////                    ss << variable_prefix << table_db_name << "." << tmpvar->id_  << variable_suffix  << " " << operator_ << " "
////                            << variable_->getValueFromRepresentation(value_, true);

//                    if (find (variable_names.begin(), variable_names.end(), tmpvar->getId()) == variable_names.end())
//                        variable_names.push_back(tmpvar->getId());

//                }
//            }
//        }
//    }

}

/**
 * Checks if value_ is different than edit_ value, if yes sets changed_ and emits possibleFilterChange.
 */
void DBFilterCondition::valueChanged ()
{
    logdbg  << "DBFilterCondition: valueChanged";
    assert  (edit_);

    if (value_.compare(edit_->text().toStdString()) != 0)
    {
        value_ = edit_->text().toStdString();

        changed_=true;

        emit possibleFilterChange();
    }
}

/**
 * Sets the variable if required, sets the variable_name_ and calls reset.
 */
void DBFilterCondition::setVariable (DBOVariable *variable)
{
    if (variable != variable_)
    {
        variable_=variable;
        variable_name_=variable_->name();

        reset();
    }
}

void DBFilterCondition::update ()
{
    assert (variable_);

    label_->setText(tr((variable_name_+" "+operator_).c_str()));
    edit_->setText (tr(value_.c_str()));
}

void DBFilterCondition::reset ()
{
    std::string value;

    if (reset_value_.compare ("MIN") == 0 || reset_value_.compare ("MAX") == 0)
    {
        // FIX MINMAX
        assert (false);
//        if (!variable_->hasMinMaxInfo())
//        {
//            value = "LOADING";
//            variable_->buildMinMaxInfo();
//        }
//        else
//        {
//            if (reset_value_.compare ("MIN") == 0)
//            {
//                value = variable_->getRepresentationFromValue(variable_->getMinString());
//                loginf << "DBFilterCondition: reset: value " << variable_->getMinString() << " repr " << value;
//            }
//            else if (reset_value_.compare ("MAX") == 0)
//            {
//                value = variable_->getRepresentationFromValue(variable_->getMaxString());
//                loginf << "DBFilterCondition: reset: value " << variable_->getMaxString() << " repr " << value;
//            }
//        }
    }
    else
        value=reset_value_;

    value_=value;

    update();
}

//void DBFilterCondition::notifyMinMax (DBOVariable *variable)
//{
//    loginf << "DBFilterCondition: notifyMinMax";

//    std::string value;

//    //TODO FIX MINMAX
//    assert (false);

//    assert (variable_->hasMinMaxInfo());
//    if (reset_value_.compare ("MIN") == 0)
//        value = variable_->getRepresentationFromValue(variable_->getMinString());
//    else if (reset_value_.compare ("MAX") == 0)
//        value = variable_->getRepresentationFromValue(variable_->getMaxString());

//    value_=value;
//    update();
//}
