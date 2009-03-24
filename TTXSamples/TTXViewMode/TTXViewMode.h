#pragma once

#include "resource.h"

#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define ORDER 4000

typedef struct {
  PTTSet ts;
  PComVar cv;
  HMENU ControlMenu;
} TInstVar;
