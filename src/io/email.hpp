/**************************************************************************
 * Copyright 2018 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_EMAIL_HPP
#define ARCMIST_EMAIL_HPP


namespace ArcMist
{
    namespace Email
    {
        bool send(const char *pFrom, const char *pTo, const char *pSubject, const char *pBody);
    }
}

#endif