#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdbool.h>
#include <string.h>

#define ERR_QUIT ""
#define RPL_WELCOME "001"
#define RPL_YOURHOST "002"
#define RPL_CREATED "003"
#define RPL_MYINFO "004"

#define RPL_LUSERCLIENT "251"
#define RPL_LUSEROP "252"
#define RPL_LUSERUNKNOWN "253"
#define RPL_LUSERCHANNELS "254"
#define RPL_LUSERME "255"

#define RPL_AWAY "301"
#define RPL_UNAWAY "305"
#define RPL_NOWAWAY "306"

#define RPL_WHOISUSER "311"
#define RPL_WHOISSERVER "312"
#define RPL_WHOISOPERATOR "313"
#define RPL_WHOISIDLE "317"
#define RPL_ENDOFWHOIS "318"
#define RPL_WHOISCHANNELS "319"

#define RPL_WHOREPLY "352"
#define RPL_ENDOFWHO "315"

#define RPL_LIST "322"
#define RPL_LISTEND "323"

#define RPL_CHANNELMODEIS "324"

#define RPL_NOTOPIC "331"
#define RPL_TOPIC "332"

#define RPL_NAMREPLY "353"
#define RPL_ENDOFNAMES "366"

#define RPL_MOTDSTART "375"
#define RPL_MOTD "372"
#define RPL_ENDOFMOTD "376"

#define RPL_YOUREOPER "381"

#define ERR_NOSUCHNICK "401"
#define ERR_NOSUCHSERVER "402"
#define ERR_NOSUCHCHANNEL "403"
#define ERR_CANNOTSENDTOCHAN "404"
#define ERR_NORECIPIENT "411"
#define ERR_NOTEXTTOSEND "412"
#define ERR_UNKNOWNCOMMAND "421"
#define ERR_NOMOTD "422"
#define ERR_NONICKNAMEGIVEN "431"
#define ERR_NICKNAMEINUSE "433"
#define ERR_USERNOTINCHANNEL "441"
#define ERR_NOTONCHANNEL "442"
#define ERR_NOTREGISTERED "451"
#define ERR_NEEDMOREPARAMS "461"
#define ERR_ALREADYREGISTRED "462"
#define ERR_PASSWDMISMATCH "464"
#define ERR_UNKNOWNMODE "472"
#define ERR_NOPRIVILEGES "481"
#define ERR_CHANOPRIVSNEEDED "482"
#define ERR_UMODEUNKNOWNFLAG "501"
#define ERR_USERSDONTMATCH "502"

bool is_error(char *command)
{
    return strcmp(command, ERR_NOSUCHNICK) == 0 || strcmp(command, ERR_NOSUCHSERVER) == 0 ||
           strcmp(command, ERR_NOSUCHCHANNEL) == 0 || strcmp(command, ERR_CANNOTSENDTOCHAN) == 0 ||
           strcmp(command, ERR_NORECIPIENT) == 0 || strcmp(command, ERR_NOTEXTTOSEND) == 0 ||
           strcmp(command, ERR_UNKNOWNCOMMAND) == 0 || strcmp(command, ERR_NOMOTD) == 0 ||
           strcmp(command, ERR_NONICKNAMEGIVEN) == 0 || strcmp(command, ERR_NICKNAMEINUSE) == 0 ||
           strcmp(command, ERR_USERNOTINCHANNEL) == 0 || strcmp(command, ERR_NOTONCHANNEL) == 0 ||
           strcmp(command, ERR_NOTREGISTERED) == 0 || strcmp(command, ERR_NEEDMOREPARAMS) == 0 ||
           strcmp(command, ERR_ALREADYREGISTRED) == 0 || strcmp(command, ERR_PASSWDMISMATCH) == 0 ||
           strcmp(command, ERR_UNKNOWNMODE) == 0 || strcmp(command, ERR_NOPRIVILEGES) == 0 ||
           strcmp(command, ERR_CHANOPRIVSNEEDED) == 0 ||
           strcmp(command, ERR_UMODEUNKNOWNFLAG) == 0 || strcmp(command, ERR_USERSDONTMATCH) == 0;
}

bool is_reply(char *command)
{
    return strcmp(command, RPL_WELCOME) == 0 || strcmp(command, RPL_YOURHOST) == 0 ||
           strcmp(command, RPL_CREATED) == 0 || strcmp(command, RPL_MYINFO) == 0 ||
           strcmp(command, RPL_LUSERCLIENT) == 0 || strcmp(command, RPL_LUSEROP) == 0 ||
           strcmp(command, RPL_LUSERUNKNOWN) == 0 || strcmp(command, RPL_LUSERCHANNELS) == 0 ||
           strcmp(command, RPL_LUSERME) == 0 || strcmp(command, RPL_AWAY) == 0 ||
           strcmp(command, RPL_UNAWAY) == 0 || strcmp(command, RPL_NOWAWAY) == 0 ||
           strcmp(command, RPL_WHOISUSER) == 0 || strcmp(command, RPL_WHOISSERVER) == 0 ||
           strcmp(command, RPL_WHOISOPERATOR) == 0 || strcmp(command, RPL_WHOISIDLE) == 0 ||
           strcmp(command, RPL_ENDOFWHOIS) == 0 || strcmp(command, RPL_WHOISCHANNELS) == 0 ||
           strcmp(command, RPL_WHOREPLY) == 0 || strcmp(command, RPL_ENDOFWHO) == 0 ||
           strcmp(command, RPL_LIST) == 0 || strcmp(command, RPL_LISTEND) == 0 ||
           strcmp(command, RPL_CHANNELMODEIS) == 0 || strcmp(command, RPL_NOTOPIC) == 0 ||
           strcmp(command, RPL_TOPIC) == 0 || strcmp(command, RPL_NAMREPLY) == 0 ||
           strcmp(command, RPL_ENDOFNAMES) == 0 || strcmp(command, RPL_MOTDSTART) == 0 ||
           strcmp(command, RPL_MOTD) == 0 || strcmp(command, RPL_ENDOFMOTD) == 0 ||
           strcmp(command, RPL_YOUREOPER) == 0;
}

#endif
