/**************************************************************************
 * Copyright 2018 ArcMist, LLC                                       *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "email.hpp"

#include "arcmist/base/log.hpp"

#include <cstdlib>

#define ARCMIST_EMAIL_LOG_NAME "Email"

namespace ArcMist
{
    namespace Email
    {
        bool send(const char *pFrom, const char *pTo, const char *pSubject, const char *pBody)
        {
            if(pTo == NULL || *pTo == 0)
                return false; // Empty to address

            FILE *mailPipe = popen("/usr/lib/sendmail -t", "w");
            if (mailPipe != NULL)
            {
                fprintf(mailPipe, "To: %s\n", pTo);
                if(pFrom != NULL) // If empty use default from address for server
                    fprintf(mailPipe, "From: %s\n", pFrom);
                fprintf(mailPipe, "Subject: %s\n\n", pSubject);
                fwrite(pBody, 1, std::strlen(pBody), mailPipe);
                fwrite("\n", 1, 2, mailPipe);
                pclose(mailPipe);
                ArcMist::Log::addFormatted(ArcMist::Log::INFO, ARCMIST_EMAIL_LOG_NAME, "Sent email : %s", pSubject);
                return true;
            }
            else
            {
                ArcMist::Log::add(ArcMist::Log::ERROR, ARCMIST_EMAIL_LOG_NAME, "Failed to send email");
                return false;
            }
        }
    }
}
