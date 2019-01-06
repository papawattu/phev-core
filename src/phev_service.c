#include <stdint.h>
#include "phev_pipe.h"
#include "phev_service.h"
#include "logger.h"
#include "cJSON.h"

const static char * APP_TAG = "PHEV_SERVICE";

phev_pipe_ctx_t * phev_service_createPipe(messagingClient_t * in, messagingClient_t * out)
{
    LOG_V(APP_TAG,"START - createPipe");

    phev_pipe_settings_t settings = {
        .ctx = NULL,
        .in = in,
        .out = out,
        .inputInputTransformer = NULL,
        .inputOutputTransformer = NULL,
        .inputSplitter = NULL,
        .outputSplitter = NULL,
        .inputResponder = NULL,
        .outputResponder = (msg_pipe_responder_t) phev_pipe_commandResponder,
        .outputOutputTransformer = (msg_pipe_transformer_t) phev_pipe_outputEventTransformer,
        .preConnectHook = NULL,
        .outputInputTransformer = (msg_pipe_transformer_t) phev_pipe_outputChainInputTransformer,
    };

    phev_pipe_ctx_t * ctx = phev_pipe_createPipe(settings);

    LOG_V(APP_TAG,"END - createPipe");
}


bool phev_service_checkByte(uint16_t num)
{
    return (num >= 0 && num < 256);
}
uint8_t phev_service_getJsonByte(cJSON * json, char * option) 
{
    cJSON * value = cJSON_GetObjectItemCaseSensitive(json, option);
    
    if(value != NULL)
    {
        return ((uint8_t) value->valueint) & 0xff;
    }
    return 0;
}

uint16_t phev_service_getJsonInt(cJSON * json, char * option) 
{
    cJSON * value = cJSON_GetObjectItemCaseSensitive(json, option);
    
    if(value != NULL)
    {
        return value->valueint;
    }
    return 0;
}
bool phev_service_validateCheckOnOrOff(const char * str)
{
    if(str)
    {
        if(strcmp(str, PHEV_SERVICE_ON_JSON) == 0)
        {
            return true;
        }
        
        if(strcmp(str, PHEV_SERVICE_OFF_JSON) == 0)
        {
            return true;
        }
    }
    
    return false;
}
bool phev_service_validateCommand(const char * command)
{
    cJSON * json = cJSON_Parse(command);
    
    if(json == NULL)
    {
        return false;
    }

    cJSON * update = cJSON_GetObjectItemCaseSensitive(json, PHEV_SERVICE_UPDATE_REGISTER_JSON);

    cJSON * operation = cJSON_GetObjectItemCaseSensitive(json, PHEV_SERVICE_OPERATION_JSON);

    if(update != NULL)
    {
        cJSON * reg = cJSON_GetObjectItemCaseSensitive(update, PHEV_SERVICE_UPDATE_REGISTER_REG_JSON);

        cJSON * value = cJSON_GetObjectItemCaseSensitive(update, PHEV_SERVICE_UPDATE_REGISTER_VALUE_JSON);
    

        if(value != NULL && reg != NULL)
        {
            return phev_service_checkByte(reg->valueint) && phev_service_checkByte(value->valueint);
        }
        
    }

    if(operation != NULL)
    {
        cJSON * headLights = cJSON_GetObjectItemCaseSensitive(operation, PHEV_SERVICE_OPERATION_HEADLIGHTS_JSON);
        
        if(headLights)
        {
            return phev_service_validateCheckOnOrOff(headLights->valuestring);
        }
        return true;
    }

    return false;
}
phevMessage_t * phev_service_operationHandler(cJSON * operation)
{
    cJSON * headLights = cJSON_GetObjectItemCaseSensitive(operation, PHEV_SERVICE_OPERATION_HEADLIGHTS_JSON);

    if(headLights)
    {
        if(strcmp(headLights->valuestring, PHEV_SERVICE_ON_JSON) == 0)
        {
            return phev_core_simpleRequestCommandMessage(KO_WF_H_LAMP_CONT_SP, 1);
        }
        if(strcmp(headLights->valuestring, PHEV_SERVICE_OFF_JSON) == 0)
        {
            return phev_core_simpleRequestCommandMessage(KO_WF_H_LAMP_CONT_SP, 2);
        }
        return NULL;
    }
    return NULL;
}
phevMessage_t * phev_service_jsonCommandToPhevMessage(const char * command)
{
    if(phev_service_validateCommand(command))
    {
        cJSON * json = cJSON_Parse(command);
    
        cJSON * update = cJSON_GetObjectItemCaseSensitive(json, PHEV_SERVICE_UPDATE_REGISTER_JSON);

        cJSON * operation = cJSON_GetObjectItemCaseSensitive(json, PHEV_SERVICE_OPERATION_JSON);

        if(update) 
        {
            return phev_core_simpleRequestCommandMessage(phev_service_getJsonByte(update,PHEV_SERVICE_UPDATE_REGISTER_REG_JSON)
                                                    ,phev_service_getJsonByte(update,PHEV_SERVICE_UPDATE_REGISTER_VALUE_JSON));
        }
        if(operation)
        {
            return phev_service_operationHandler(operation);
        }
    } else {
        return NULL;
    }
}
message_t * phev_service_jsonInputTransformer(message_t * message)
{
    if(message)
    {
        return phev_core_convertToMessage(phev_service_jsonCommandToPhevMessage(message->data));
    }
    return NULL;
}
