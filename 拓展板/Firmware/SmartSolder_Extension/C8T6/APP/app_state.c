#include "app_state.h"
#include <string.h>

void AppState_Init(AppState_t *state)
{
    if (state == 0)
    {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->mode = APP_MODE_MANUAL;
    state->ironTemp = -1;
    state->setTemp = 350;
    strcpy(state->ip, "0.0.0.0");
    strcpy(state->lastDebug, "boot");
}

const char *AppMode_ToText(AppMode_t mode)
{
    return (mode == APP_MODE_AUTO) ? "AUTO" : "MANUAL";
}
