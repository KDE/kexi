/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KEXIRELATIONDESIGNSHAPE_H
#define KEXIRELATIONDESIGNSHAPE_H

#include <KoShape.h>
#include <KUrl>

namespace KexiDB {
class Connection;
class ConnectionData;
class TableOrQuerySchema;
}

#define KEXIRELATIONDEISGNSHAPEID "KexiRelationDesignShape"

class KexiRelationDesignShape : public KoShape {
    public:
        KexiRelationDesignShape();
        ~KexiRelationDesignShape();
        
        virtual void saveOdf ( KoShapeSavingContext& context ) const;
        virtual bool loadOdf ( const KoXmlElement& element, KoShapeLoadingContext& context );
        virtual void paint ( QPainter& painter, const KoViewConverter& converter );

        void setConnectionData(KexiDB::ConnectionData*);
        KexiDB::ConnectionData* connectionData();

        void setRelation(const QString&);
        
        KexiDB::Connection* connection();
        
    private:
        KexiDB::ConnectionData *m_connectionData;
        KexiDB::Connection *m_connection;
        QString m_relation;
        KexiDB::TableOrQuerySchema* m_relationSchema;
};

#endif // KEXIRELATIONDESIGNSHAPE_H
