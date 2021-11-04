#ifndef _MESSAGE_H
#define _MESSAGE_H

typedef struct Message
{
    int code;
    char *argument;
} Message;

typedef struct Payload
{
    int code;
    int length;
} Payload;

#endif
