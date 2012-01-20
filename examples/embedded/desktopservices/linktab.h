/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef LINKTAB_H_
#define LINKTAB_H_

// EXTERNAL INCLUDES

// INTERNAL INCLUDES
#include "contenttab.h"

// FORWARD DECLARATIONS
QT_BEGIN_NAMESPACE
class QWidget;
class QListWidgetItem;
QT_END_NAMESPACE

// CLASS DECLARATION

/**
* LinkTab class.
*
* This class implements tab for opening http and mailto links.
*/
class LinkTab : public ContentTab
{
    Q_OBJECT

public:     // Constructors & Destructors
    LinkTab(QWidget *parent);
    ~LinkTab();

protected:  // Derived Methods
    virtual void populateListWidget();
    virtual QUrl itemUrl(QListWidgetItem *item);
    virtual void handleErrorInOpen(QListWidgetItem *item);

private:    // Used variables
    QListWidgetItem *m_WebItem;
    QListWidgetItem *m_MailToItem;

private:    // Owned variables

};

#endif // CONTENTTAB_H_

// End of File
