// =================================================================================================
///  @file R1Statement.h
///
///  RyftOne DSIStatement implementation.
///
///  Copyright (C) 2005-2014 Simba Technologies Incorporated.
// =================================================================================================

#ifndef _RYFTONE_R1STATEMENT_H_
#define _RYFTONE_R1STATEMENT_H_

#include "RyftOne.h"
#include "R1Catalog.h"
#include "DSIStatement.h"

namespace RyftOne
{
    /// @brief RyftOne DSIStatement implementation class.
    class R1Statement : public Simba::DSI::DSIStatement
    {
    // Public ======================================================================================
    public:
        /// @brief Constructor.
        /// 
        /// Default property values and limits can be overridden here.
        /// 
        /// @param in_connection            The parent connection (NOT OWN).
        R1Statement(Simba::DSI::IConnection* in_connection, RyftOne_Database *ryft1);

        /// @brief Destructor.
        ~R1Statement();

        /// @brief Constructs a new data engine instance.
        ///
        /// @return A new IDataEngine instance. (OWN)
        virtual Simba::DSI::IDataEngine* CreateDataEngine();

    protected:
        RyftOne_Database *m_ryft1;  
    };
}

#endif
