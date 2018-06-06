/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_EMAIL_HPP
#define NEXTCASH_EMAIL_HPP


namespace NextCash
{
    namespace Email
    {
        bool send(const char *pFrom, const char *pTo, const char *pSubject, const char *pBody);
    }
}

#endif