#include "TexUtils.h"

void TexUtils::clearTexture(SDL_Renderer* rend, SDL_Texture*& tex)
{
    //Save old render target, draw blend mode, and draw color
    SDL_Texture* oldRTarget = SDL_GetRenderTarget(rend);
    SDL_BlendMode oldRDBlendMode;   SDL_GetRenderDrawBlendMode(rend, &oldRDBlendMode);
    uint8_t oRDR, oRDG, oRDB, oRDA; SDL_GetRenderDrawColor(rend, &oRDR, &oRDG, &oRDB, &oRDA);

    //Clear the texture
    SDL_SetRenderTarget(rend, tex);                         //Set render target to tex we are clearing
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE);   //Set blend mode to SDL_BLENDMODE_NONE to replace all pixels with transparency
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);               //Set render draw color to invisible
    SDL_RenderFillRect(rend, NULL);                         //Fill tex with transparency

    //Restore old render target, draw blend mode, and draw color
    SDL_SetRenderTarget(rend, oldRTarget);
    SDL_SetRenderDrawBlendMode(rend, oldRDBlendMode);
    SDL_SetRenderDrawColor(rend, oRDR, oRDG, oRDB, oRDA);
}