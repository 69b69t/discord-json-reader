#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

uint32_t getFileLength(FILE * file)
{
    uint32_t pos;
    fseek(file, 0, SEEK_END);
    pos = ftell(file);
    return pos;
}

cJSON* returnParsedJson(char* fileName)
{
    //LOADING AND WRITING TO MEMORY
    FILE * jsonFile = fopen(fileName, "r");
    uint32_t fileLength = getFileLength(jsonFile);

    char * json = malloc(fileLength * sizeof(char));

    if (json == NULL) {
        printf("failed allocation\n");
        exit(-1);
    }

    fseek(jsonFile, 0, SEEK_SET);
    for(uint32_t i = 0; i < fileLength && !feof(jsonFile); i++)
    {
        json[i] = getc(jsonFile);
    }
    json[fileLength-1] = '\0';
    fclose(jsonFile);
    //END LOADING AND ADDING TO MEMORY

    cJSON *jsonParsed = cJSON_Parse(json);

    //dont need json stuff anymore
    free(json);
    //did json ACTUALLY parse???
    if (jsonParsed == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) fprintf(stderr, "Error before: %s\n", error_ptr);
        else fprintf(stderr, "Error: cJSON_Parse failed. Invalid JSON.\n");
        exit(-1);
    }

    return jsonParsed;
}

cJSON* findRepliedMessage(cJSON* singleMessage)
{
    //this gets called on replies.
    //it will check if the "reference" field exists in the message. error if not.
    //the reference field will have a key called "messageId"
    //take messageId and go back messages until the message id matches messageId

    cJSON *type = cJSON_GetObjectItem(singleMessage, "type");

    // message is not a reply
    if (type == NULL || type->type != cJSON_String || strcmp(type->valuestring, "Reply") != 0) {
        //printf("Not a Reply message.\n");
        return NULL;
    }

    // find "reference" key. if it does not exist, return null
    cJSON *refField = cJSON_GetObjectItem(singleMessage, "reference");
    if (refField == NULL) {
        printf("\"reference\" field not found.\n");
        return NULL;
    }
    
    // grab the messageId: "897346592387" looking section. if it returns null fail
    cJSON *messageId = cJSON_GetObjectItem(refField, "messageId");
    if (messageId == NULL) {
        //cJSON *reference = cJSON_GetObjectItem(singleMessage, "reference");
        printf("\"messageId\" field not found in the reference:\n%s\n", cJSON_Print(messageId));
        return NULL;
    }

    //get the double from there
    char* id = messageId->valuestring;
    
    cJSON *currentMessage = singleMessage;
    while(strcmp(id, cJSON_GetObjectItem(currentMessage, "id")->valuestring) != 0 || currentMessage == NULL)
    {
        currentMessage = currentMessage->prev;
    }

    return currentMessage;
}

int main(int argc, char** argv)
{
    cJSON* jsonParsed = returnParsedJson(argv[1]);
    //get the array labled messages
    cJSON *messagesArray = cJSON_GetObjectItem(jsonParsed, "messages");

    //traverse into the array
    cJSON *current = messagesArray->child;

    cJSON *repliedMessage;

    while (current != NULL)
    {
        //print timestamp
        printf("%s ", cJSON_GetObjectItem(current, "timestamp")->valuestring);

        //print user
        printf("%s: ", cJSON_GetObjectItem(cJSON_GetObjectItem(current, "author"), "name")->valuestring);

        //print message content
        printf("%s", cJSON_GetObjectItem(current, "content")->valuestring);

        //if this message is a reply, return the message it replied to. otherwise, return NULL
        repliedMessage = findRepliedMessage(current);
        if(repliedMessage != NULL)
        {
            printf(" (replied to %s", cJSON_GetObjectItem(cJSON_GetObjectItem(repliedMessage, "author"), "name")->valuestring);
            printf(": %s)", cJSON_GetObjectItem(repliedMessage, "content")->valuestring);
        }

        //message seperator
        printf("\n");
        current = current->next;
    }

    //recursively frees json
    cJSON_Delete(jsonParsed);
}