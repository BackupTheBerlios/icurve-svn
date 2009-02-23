/* Integral Curve
 *  Copyright (c) Bartosz Pastudzki, 2009
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 * Program draws integral curves.
 * utf-8, c99
 * requires SDL, SDL_draw i SDL_ttf*/

#include <SDL/SDL.h>
#include <SDL/SDL_draw.h>
#include <SDL/SDL_ttf.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef unsigned int uint;
typedef unsigned char uchar;
#define ALLOC(TYPE, COUNT) (TYPE*) malloc(sizeof(TYPE)*(COUNT))

//Image options.
#define RESX 1024
#define RESY 768
#define BPP 24
#define CURVES_AT_ONCE 100
#define FUNCTIONS_FILE "functions.txt"

//Count of pixels per one unit @ scale=1.0
#define PPU 40

//Only font used in program. Initialized in main()
TTF_Font *font;

//Function that draws surface on another one on position (x,y)
void moveLayer(int x, int y, SDL_Surface *src, SDL_Surface *dest);

//Function that draws coordinate system with grid.
void drawCoordinateSystem(SDL_Surface *dest, double xCenter, double yCenter,
                             double scale);

void refreshImage(SDL_Surface *screen, double xCenter, double yCenter,
                   double scale);

void drawFunction(SDL_Surface *screen, double xCenter, double yCenter,
                  double scale, double x, double y, char *function);

double calculate(char *function, double x, double y);
double calculateParenthesis(char **function, double x, double y);

char *currentFunction;
char **functions;
uint functionsCount;
uint functionsNumber;

typedef struct
{
  char *function;
  double x, y;
}Curve;

Curve *curves;
int lastCurve;

//------------------------------------------------------------------------------
int main(void){
  char buffer[1001];
  FILE *funcs = fopen(FUNCTIONS_FILE, "r");
  functionsCount = 0;
  while(1)  {
    if(fgets(buffer, 1000, funcs)==NULL)
      break;
    functionsCount++;
  }
  funcs = fopen(FUNCTIONS_FILE, "r");
  functions = ALLOC(char*, functionsCount);
  for(uint i = functionsCount; i>0;){
    fgets(buffer, 1000, funcs);
    functions[--i] = ALLOC(char, strlen(buffer));
    memcpy(functions[i], buffer, strlen(buffer));
  }
  currentFunction = functions[0];
  functionsNumber = 0;

  curves = ALLOC(Curve, CURVES_AT_ONCE);
  for(uint i = CURVES_AT_ONCE; i>0;){
    curves[--i].function=NULL;
  }

  lastCurve = -1;

  if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != -1){
    if(TTF_Init() != -1){
      SDL_Surface *screen = SDL_SetVideoMode(RESX,RESY,BPP,SDL_SWSURFACE/*|SDL_FULLSCREEN*/);
      if(screen != NULL){
        SDL_WM_SetCaption("Integral Curves", NULL);
        font = TTF_OpenFont("DejaVuSans.ttf", 12);
        double xCenter = 0, yCenter = 0, scale = 1;
        refreshImage(screen, xCenter, yCenter, scale);

        bool endOfLoop = false;
        while(endOfLoop==false){
          SDL_Event event;
          SDL_PollEvent(&event);
          switch(event.type){
          case SDL_QUIT:
            endOfLoop = true;
            break;
          case SDL_MOUSEBUTTONDOWN:
            {
              int x = event.button.x, y = event.button.y;

              //Quit button
              if(x>=RESX-50 && y<=17){
                endOfLoop=true;
                break;
              }
              //Zoom in
              if(x<=40 && y<=19 && scale <= 15){
                scale*=1.1;
                refreshImage(screen, xCenter, yCenter, scale);
                break;
              }
              //Zoom out
              if(x<=40 && y<= 35 && scale >= 0.2){
                scale*=0.90;
                refreshImage(screen, xCenter, yCenter, scale);
                break;
              }
              if(x>=75 && x <=85){
                //X+
                if(y>=8 && y <= 18){
                  xCenter+=1/scale;
                  refreshImage(screen, xCenter, yCenter, scale);
                  break;
                }
                //Y+
                if(y>=23 && y <= 33){
                  yCenter+=1/scale;
                  refreshImage(screen, xCenter, yCenter, scale);
                  break;
                }
              }
              if(x>=87 && x <=97){
                //X-
                if(y>=8 && y <= 18){
                  xCenter-=1/scale;
                  refreshImage(screen, xCenter, yCenter, scale);
                  break;
                }
                //Y-
                if(y>=23 && y <= 33){
                  yCenter-=1/scale;
                  refreshImage(screen, xCenter, yCenter, scale);
                  break;
                }
              }
              //clear
              if(x>=110 && x<=140 && y<=25){
                for(uint i = CURVES_AT_ONCE; i>0;)
                  curves[--i].function=NULL;
                lastCurve = -1;
                refreshImage(screen, xCenter, yCenter, scale);
                break;
              }
              //change function
              if(y<=25 && x >= 190 && x <= 250){
                if(event.button.button == SDL_BUTTON_LEFT)
                  functionsNumber = (functionsNumber + 1) % functionsCount;
                else
                  functionsNumber = (functionsNumber + functionsCount - 1)%
                                    functionsCount;
                currentFunction = functions[functionsNumber];
                refreshImage(screen, xCenter, yCenter, scale);
                break;
              }
              //drawing
              if(++lastCurve == CURVES_AT_ONCE)
                lastCurve = 0;
              curves[lastCurve].function = currentFunction;
              curves[lastCurve].x = xCenter-((RESX/2.)-x)/(PPU*scale);
              curves[lastCurve].y = yCenter+((RESY/2.)-y)/(PPU*scale);
              refreshImage(screen, xCenter, yCenter, scale);
              break;
            }//End of case
          }  //End of switch
        }    //End of while

        TTF_CloseFont(font);
      }
    }
    TTF_Quit();
  }
  SDL_Quit();
  fclose(funcs);
  return 0;
}

//------------------------------------------------------------------------------
void moveLayer(int x, int y, SDL_Surface *src, SDL_Surface *dest){
  SDL_Rect offset; 
  offset.x = x;
  offset.y = y; 
  SDL_BlitSurface(src, NULL, dest, &offset ); 
}

//------------------------------------------------------------------------------
void drawCoordinateSystem(SDL_Surface *layer, double xCenter, double yCenter,
                             double scale){
  Uint32 bg = SDL_MapRGB(layer->format, 0 ,0, 0),
         axes = SDL_MapRGB(layer->format, 0xd0, 0xd0, 0xd0),
         grid = SDL_MapRGB(layer->format, 0x20, 0x20, 0x20);

  Draw_FillRect(layer, 0,0,RESX,RESY,bg);

  uint rozstawSiatki = PPU * scale;
  //position of axes:
  int xAxis = (RESY/2)+(int)(yCenter*PPU*scale);
  int yAxis = (RESX/2)-(int)(xCenter*PPU*scale);

  for(uint x = abs(yAxis)%rozstawSiatki; x<RESX; x+=rozstawSiatki)
    Draw_Line(layer, x, 0, x, RESY-1, grid);

  for(uint y = abs(xAxis)%rozstawSiatki; y<RESY; y+=rozstawSiatki)
    Draw_Line(layer, 0, y, RESX-1, y, grid);

  if(yAxis<RESX && yAxis >= 0){
    Draw_Line(layer, yAxis, 0, yAxis, RESY-1, axes);
    Draw_Line(layer, yAxis, 0, yAxis-2, 5, axes);
    Draw_Line(layer, yAxis, 0, yAxis+2, 5, axes);
  }

  if(xAxis<RESY && xAxis >= 0){
    Draw_Line(layer, 0, xAxis, RESX-1, xAxis, axes);
    Draw_Line(layer, RESX-1, xAxis, RESX-5, xAxis-2, axes);
    Draw_Line(layer, RESX-1, xAxis, RESX-5, xAxis+2, axes);
  }
}

//------------------------------------------------------------------------------
void refreshImage(SDL_Surface *screen, double xCenter, double yCenter,
                   double scale){
  drawCoordinateSystem(screen, xCenter, yCenter, scale);
  SDL_Color textColor;
  textColor.r = 0xff;
  textColor.g = 0xff;
  textColor.b = 0xff;
  SDL_Surface *tmp = TTF_RenderText_Solid(font, "Close", textColor);
  moveLayer(RESX-50, 5, tmp, screen);
  SDL_FreeSurface(tmp);
  tmp = TTF_RenderText_Solid(font, "Zoom in", textColor);
  moveLayer(5,5, tmp, screen);
  SDL_FreeSurface(tmp);
  tmp = TTF_RenderText_Solid(font, "Zoom out", textColor);
  moveLayer(5,20, tmp, screen);
  SDL_FreeSurface(tmp);
  tmp = TTF_RenderText_Solid(font, "X + -", textColor);
  moveLayer(65,5, tmp, screen);
  SDL_FreeSurface(tmp);
  tmp = TTF_RenderText_Solid(font, "Y + -", textColor);
  moveLayer(65,20, tmp, screen);
  SDL_FreeSurface(tmp);
  tmp = TTF_RenderText_Solid(font, "Clear", textColor);
  moveLayer(110, 5, tmp, screen);
  SDL_FreeSurface(tmp);
  tmp = TTF_RenderText_Solid(font, "f'(x,y)=", textColor);
  moveLayer(190, 5, tmp, screen);
  SDL_FreeSurface(tmp);
  tmp = TTF_RenderText_Solid(font, currentFunction, textColor);
  moveLayer(240, 5, tmp, screen);
  for(unsigned int i = 0; i<CURVES_AT_ONCE;i++){
    if(curves[i].function == NULL)
      break;
    drawFunction(screen, xCenter, yCenter, scale, curves[i].x, curves[i].y, curves[i].function);
  }
  SDL_FreeSurface(tmp);
  SDL_Flip(screen);
}

//------------------------------------------------------------------------------
void drawFunction(SDL_Surface *screen, double xCenter, double yCenter,
                  double scale, double x, double y, char *function){
  Uint32 color = SDL_MapRGB(screen->format, 0xff, 0xff, 0x00);
  double x0=x, y0=y, step = 1/(PPU*scale);

  while(1){
    double deriviative = calculate(function, x, y);
    if(isnan(deriviative))
      break;

    uint px = (RESX/2)-((xCenter-x)*scale*PPU),
         newX = px + 1,
         py = (RESY/2)+((yCenter-y)*scale*PPU),
         newY = py - 1*deriviative;

    if(newX>RESX || x>RESX || newY>RESY || y>RESY)
      break;

    Draw_Line(screen, px, py, newX, newY, color);
    x += step;
    y += step*deriviative;
  }
  x = x0;
  y = y0;
  while(1){
    double deriviative = calculate(currentFunction, x, y);
    if(isnan(deriviative))
      break;

    uint px = (RESX/2)-((xCenter-x)*scale*PPU),
         newX = px - 1,
         py = (RESY/2)+((yCenter-y)*scale*PPU),
         newY = py + 1*deriviative;

    if(newX>RESX || x>RESX || newY>RESY || y>RESY)
      break;

    Draw_Line(screen, px, py, newX, newY, color);
    x -= step;
    y -= step*deriviative;
  }
}

//------------------------------------------------------------------------------
double calculate(char *function, double x, double y){
  double result = 0.0;
  int multiplier = 1;
  while(1){
    double monomianResult = 0;
    if(*function > '9' || *function < '0')
      monomianResult = 1;
    else while(*function >= '0' && *function<='9'){
      monomianResult *= 10;
      monomianResult += *function - '0';
      function++;
    }
    while(1){
      if(*function == '('){
        double result = calculateParenthesis(&function, x,y);
        if(isnan(result))
          return result;
        else monomianResult *= result;
        continue;
      }
      if(*function == 'x'){
        uint power;
        if(*(++function) == '^'){
          power = *(++function) - '0';
          function++;
        }
        else power = 1;
        monomianResult *= pow(x, power);
        continue;
      }
      if(*function == 'y'){
        uint power;
        if(*(++function) == '^'){
          power = *(++function) - '0';
          function++;
        }
        else power = 1;
        monomianResult *= pow(y, power);
        continue;
      }
      if(*function == '/' && *(++function) == '('){
        double divisor = calculateParenthesis(&function, x, y);
        if(isnan(divisor))
          return divisor;
        if(fabs(divisor) <= 0.01)
          return 0./0.;
        else
        monomianResult /= divisor;
        continue;
      }
      if(*function == 'l' && *(++function) == 'n'){
        uint power;
        if(*(++function) == '^'){
          power = *(++function) - '0';
          function++;
        }
        else power = 1;
        double logarithmedNumber = calculateParenthesis(&function, x, y);
        if(isnan(logarithmedNumber))
          return logarithmedNumber;
        if(fabs(logarithmedNumber) < 0.01)
          return 0.0/0.0;
        else
          monomianResult *= pow(log(logarithmedNumber),power);
        continue;
      }
      if(*function == 'a' && *(++function) == 'b' && *(++function) == 's'){
        function++;
        monomianResult *= fabs(calculateParenthesis(&function, x, y));
        continue;
      }
      if(*function == 's' && *(++function) == 'i' && *(++function) == 'n'){
        uint power;
        if(*(++function) == '^'){
          power = *(++function) - '0';
          function++;
        }
        else power = 1;
        double argument = calculateParenthesis(&function,x,y);
        if(isnan(argument))
          return argument;
        else
          monomianResult *= pow(sin(argument),power);
        continue;
      }
      if(*function == 'c' && *(++function) == 'o' && *(++function) == 's'){
        uint power;
        if(*(++function) == '^'){
          power = *(++function) - '0';
          function++;
        }
        else power = 1;
        double argument = calculateParenthesis(&function,x,y);
        if(isnan(argument))
          return argument;
        monomianResult *= pow(cos(argument),power);
        continue;
      }
      if(*function == '+'){
        function++;
        result += multiplier*monomianResult;
        multiplier = 1;
        break;
      }
      if(*function == '-'){
        function++;
        result += multiplier*monomianResult;
        multiplier = -1;
        break;
      }
      else{
        result += multiplier*monomianResult;
        return result;
      }
    }
  }
}

//------------------------------------------------------------------------------
double calculateParenthesis(char **function, double x, double y){
  char *cursor = ++(*function);
  while(*cursor != ')')
    cursor++;
  char *functionWewn = ALLOC(char, cursor-*function+1);
  memcpy(functionWewn, *function, cursor-*function+1);
  double result = calculate(functionWewn, x, y);
  free(functionWewn);
  *function = cursor+1;
  return result;
}

//EOF
