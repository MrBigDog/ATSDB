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
 * DBObjectEditWidget.h
 *
 *  Created on: Aug 27, 2012
 *      Author: sk
 */

#ifndef DBOBJECTINFOWIDGET_H_
#define DBOBJECTINFOWIDGET_H_

#include <QWidget>
#include <map>

class QLabel;
class QCheckBox;
class QPushButton;
class QVBoxLayout;

class DBObject;

/**
 * @brief Edit widget for a DBObject
 */
class DBObjectInfoWidget : public QWidget
{
    Q_OBJECT

public slots:
    void loadChangedSlot();
    void updateSlot();

public:
    /// @brief Constructor
    DBObjectInfoWidget(DBObject &object, QWidget * parent = 0, Qt::WindowFlags f = 0);
    /// @brief Destructor
    virtual ~DBObjectInfoWidget();

private:
    /// @brief DBObject to be managed
    DBObject &object_;

    QVBoxLayout *main_layout_;
    QCheckBox *main_check_;

    QLabel *status_label_;
    QLabel *total_count_label_;
    QLabel *loaded_count_label_;
};

#endif /* DBOBJECTINFOWIDGET_H_ */