/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmltypenode.h"
#include "collectionnode.h"
#include "qdocdatabase.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QMultiMap<const Node *, Node *> QmlTypeNode::s_inheritedBy;

/*!
  Constructs a Qml type node or a Js type node depending on
  the value of \a type, which is Node::QmlType by default,
  but which can also be Node::JsType. The new node has the
  given \a parent and \a name.
 */
QmlTypeNode::QmlTypeNode(Aggregate *parent, const QString &name, NodeType type)
    : Aggregate(type, parent, name)
{
    setTitle(name);
}

/*!
  Clear the static maps so that subsequent runs don't try to use
  contents from a previous run.
 */
void QmlTypeNode::terminate()
{
    s_inheritedBy.clear();
}

/*!
  Record the fact that QML class \a base is inherited by
  QML class \a sub.
 */
void QmlTypeNode::addInheritedBy(const Node *base, Node *sub)
{
    if (sub->isInternal())
        return;
    if (!s_inheritedBy.contains(base, sub))
        s_inheritedBy.insert(base, sub);
}

/*!
  Loads the list \a subs with the nodes of all the subclasses of \a base.
 */
void QmlTypeNode::subclasses(const Node *base, NodeList &subs)
{
    subs.clear();
    if (s_inheritedBy.count(base) > 0) {
        subs = s_inheritedBy.values(base);
    }
}

/*!
  If this QML type node has a base type node,
  return the fully qualified name of that QML
  type, i.e. <QML-module-name>::<QML-type-name>.
 */
QString QmlTypeNode::qmlFullBaseName() const
{
    QString result;
    if (m_qmlBaseNode) {
        result = m_qmlBaseNode->logicalModuleName() + "::" + m_qmlBaseNode->name();
    }
    return result;
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module name from the QML module node. Otherwise, return the
  empty string.
 */
QString QmlTypeNode::logicalModuleName() const
{
    return (m_logicalModule ? m_logicalModule->logicalModuleName() : QString());
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module version from the QML module node. Otherwise, return
  the empty string.
 */
QString QmlTypeNode::logicalModuleVersion() const
{
    return (m_logicalModule ? m_logicalModule->logicalModuleVersion() : QString());
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module identifier from the QML module node. Otherwise, return
  the empty string.
 */
QString QmlTypeNode::logicalModuleIdentifier() const
{
    return (m_logicalModule ? m_logicalModule->logicalModuleIdentifier() : QString());
}

/*!
  Returns true if this QML type inherits \a type.
 */
bool QmlTypeNode::inherits(Aggregate *type)
{
    QmlTypeNode *qtn = qmlBaseNode();
    while (qtn != nullptr) {
        if (qtn == type)
            return true;
        qtn = qtn->qmlBaseNode();
    }
    return false;
}

/*!
  Recursively resolves the base node for this QML type when only the name of
  the base type is known.

  \a previousSearches is used for speeding up the process.
*/
void QmlTypeNode::resolveInheritance(NodeMap &previousSearches)
{
    if (m_qmlBaseNode || m_qmlBaseName.isEmpty())
        return;

    auto *base = static_cast<QmlTypeNode *>(previousSearches.value(m_qmlBaseName));
    if (!previousSearches.contains(m_qmlBaseName)) {
        for (const auto &import : qAsConst(m_importList)) {
            base = QDocDatabase::qdocDB()->findQmlType(import, m_qmlBaseName);
            if (base)
                break;
        }
        if (!base) {
            if (m_qmlBaseName.contains(':'))
                base = QDocDatabase::qdocDB()->findQmlType(m_qmlBaseName);
            else
                base = QDocDatabase::qdocDB()->findQmlType(QString(), m_qmlBaseName);
        }
        previousSearches.insert(m_qmlBaseName, base);
    }

    if (base && base != this) {
        m_qmlBaseNode = base;
        QmlTypeNode::addInheritedBy(base, this);
        // Base types read from the index need resolving as they only have the name set
        if (base->isIndexNode())
            base->resolveInheritance(previousSearches);
    }
}

/*!
  Constructs either a Qml basic type node or a Javascript
  basic type node, depending on the value pf \a type, which
  must be either Node::QmlBasicType or Node::JsBasicType.
  The new node has the given \a parent and \a name.
 */
QmlValueTypeNode::QmlValueTypeNode(Aggregate *parent, const QString &name, Node::NodeType type)
    : Aggregate(type, parent, name)
{
    setTitle(name);
}

QT_END_NAMESPACE
