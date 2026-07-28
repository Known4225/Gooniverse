/*
Created by Ryan Sricha, 28.07.26

TODO:
- Color based on group
- Algorithm to determine position of nodes (possibly multiple algorithms)
*/

#include "turtle.h"
#include <time.h>

int8_t streq(const char *str1, const char *str2);
int32_t loadSquadFile(char *filename);
void removeOverlap();

enum {
    CA_NAME = 0, // only first name (string)
    CA_DESCRIPTION = 1, // all information in the character file prior to the connections list (string)
    CA_BIRTHYEAR = 2, // int
    CA_BIRTHMONTH = 3, // int (1 to 12)
    CA_BIRTHDAY = 4, // int (1 to 31)
    CA_MENTIONED = 5, // int
    CA_CONNECTIONS = 6, // list, see CO_X
    CA_XPOS = 7, // double
    CA_YPOS = 8, // double
    CA_SIZE = 9, // double
    CA_NUMBER_OF_FIELDS = 10,
};

enum {
    CO_NAME = 0, // only first name (string)
    CO_INDEX = 1, // int
    CO_DESCRIPTION = 2, // string
    CO_NUMBER_OF_FIELDS = 3,
};

char months[12][32] = {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
};

enum {
    KEYS_LMB,
    KEYS_RMB,
    KEYS_SPACE,
};

typedef struct {
    list_t *characters;
    double screenX;
    double screenY;
    double zoom;
    double scrollSpeed;
    int32_t mouseHover;
    int32_t mouseDragging;
    double anchorX;
    double anchorY;
    double anchorMouseX;
    double anchorMouseY;
    int8_t keys[32];
} squad_t;

squad_t self;

void init() {
    self.characters = list_init();
    self.screenX = 0;
    self.screenY = 0;
    self.zoom = 1;
    self.scrollSpeed = 1.15;
    self.mouseHover = -1;
    self.mouseDragging = -1;
    /* load files */
    char *constructedFilepath = malloc(5120);
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "characters");
    list_t *squadFiles = osToolsFileList(constructedFilepath);
    for (int32_t i = 0; i < squadFiles -> length; i += 2) {
        char *name = squadFiles -> data[i].s;
        int32_t length = strlen(name);
        if (length > 2 && name[length - 1] == 'q' && name[length - 2] == 's' && name[length - 3] == '.') {
            strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
            strcat(constructedFilepath, "characters/");
            strcat(constructedFilepath, name);
            loadSquadFile(constructedFilepath);
        }
    }
    free(constructedFilepath);
    /* update indices */
    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
        list_t *connections = self.characters -> data[characterIndex + CA_CONNECTIONS].r;
        for (int32_t connectionIndex = 0; connectionIndex < connections -> length; connectionIndex += CO_NUMBER_OF_FIELDS) {
            char *name = connections -> data[connectionIndex + CO_NAME].s;
            for (int32_t characterIndexInner = 0; characterIndexInner < self.characters -> length; characterIndexInner += CA_NUMBER_OF_FIELDS) {
                if (streq(name, self.characters -> data[characterIndexInner + CA_NAME].s)) {
                    /* match */
                    connections -> data[connectionIndex + CO_INDEX].i = characterIndexInner;
                    break;
                }
            }
        }
    }
    /* run algorithms - TODO */
    removeOverlap();
}

int8_t streq(const char *str1, const char *str2) {
    if (strcmp(str1, str2) == 0) {
        return 1;
    }
    return 0;
}

int32_t loadSquadFile(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("ERROR: Could not open file %s\n", filename);
        return -1;
    }
    char firstName[32] = "NULL";
    int32_t fileLength = strlen(filename);
    for (int32_t i = fileLength; i > -1; i--) {
        if (filename[i] == '.') {
            filename[i] = '\0';
        }
        if (filename[i] == '/' || filename[i] == '\\') {
            if (fileLength - i < 32) {
                strcpy(firstName, filename + i + 1);
                break;
            }
        }
    }
    /* populate character list */
    int32_t characterIndex = self.characters -> length;
    list_append(self.characters, (unitype) firstName, 's'); // CA_NAME
    list_append(self.characters, (unitype) "NULL - to be replaced", 's'); // CA_DESCRIPTION
    list_append(self.characters, (unitype) 1900, 'i'); // CA_BIRTHYEAR
    list_append(self.characters, (unitype) 1, 'i'); // CA_BIRTHMONTH
    list_append(self.characters, (unitype) 1, 'i'); // CA_BIRTHDAY
    list_append(self.characters, (unitype) 0, 'i'); // CA_MENTIONED
    list_t *connections = list_init();
    list_append(self.characters, (unitype) connections, 'r'); // CA_CONNECTIONS
    list_append(self.characters, (unitype) randomDouble(-60, 60), 'd'); // CA_XPOS
    list_append(self.characters, (unitype) randomDouble(-60, 60), 'd'); // CA_YPOS
    list_append(self.characters, (unitype) 10.0, 'd'); // CA_SIZE
    /* read file */
    char *lineBuffer = malloc(4096);
    char *description = malloc(8192);
    description[0] = '\0';
    int32_t mode = 0;
    int32_t line = 0;
    while (fgets(lineBuffer, 4096, fp) != NULL) {
        while (strlen(lineBuffer) > 0 && (lineBuffer[strlen(lineBuffer) - 1] == '\r' || lineBuffer[strlen(lineBuffer) - 1] == '\n')) {
            lineBuffer[strlen(lineBuffer) - 1] = '\0';
        }
        if (streq(lineBuffer, "# Connections")) {
            free(self.characters -> data[characterIndex + CA_DESCRIPTION].s);
            self.characters -> data[characterIndex + CA_DESCRIPTION].s = description;
            mode = 1;
            continue;
        }
        if (mode == 0) {
            if (line == 0) {
                /* full name */
            } else if (line == 1) {
                /* birthday */
                
                if (0) {
                    if (0) {
                        /* month, day, and year */
                    } else {
                        /* just month and year */
                    }
                } else {
                    /* just year */
                    sscanf(lineBuffer + strlen("Born: "), "%d", &self.characters -> data[characterIndex + CA_BIRTHYEAR].i);
                }
            } else if (line == 2) {
                /* mentioned */
                sscanf(lineBuffer + strlen("Mentioned "), "%d", &self.characters -> data[characterIndex + CA_MENTIONED].i);
                self.characters -> data[characterIndex + CA_SIZE].d = log(self.characters -> data[characterIndex + CA_MENTIONED].i + 1) * 4 + 10;
            }
            /* description */
            strcat(description, lineBuffer);
            strcat(description, "\n");
            line++;
        } else {
            /* connection */
            char *line = lineBuffer + 2; // skip "- "
            int32_t length = strlen(line);
            int32_t found = -1;
            for (int32_t i = 0; i < length; i++) {
                if (line[i] == ',') {
                    found = i;
                    break;
                }
            }
            if (found != -1) {
                char *name = line;
                name[found] = '\0';
                char *description = line + found + 2;
                list_append(connections, (unitype) name, 's'); // CO_NAME
                list_append(connections, (unitype) -1, 'i'); // CO_INDEX
                list_append(connections, (unitype) description, 's'); // CO_DESCRIPTION
            }
        }
    }
    fclose(fp);
    free(lineBuffer);
    return 0;
}

void removeOverlap() {
    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
        /* sweep ten times */
        for (int32_t k = 0; k < 10; k++) {
            for (int32_t characterIndexInner = 0; characterIndexInner < self.characters -> length; characterIndexInner += CA_NUMBER_OF_FIELDS) {
                double xdist = self.characters -> data[characterIndexInner + CA_XPOS].d - self.characters -> data[characterIndex + CA_XPOS].d;
                double ydist = self.characters -> data[characterIndexInner + CA_YPOS].d - self.characters -> data[characterIndex + CA_YPOS].d;
                double combinedSize = self.characters -> data[characterIndexInner + CA_SIZE].d / 2 + self.characters -> data[characterIndex + CA_SIZE].d / 2;
                if (xdist * xdist + ydist * ydist < combinedSize * combinedSize && characterIndex != characterIndexInner) {
                    /* nodes are touching */
                    double theta = randomDouble(0, 360);
                    if (self.characters -> data[characterIndex + CA_SIZE].d > self.characters -> data[characterIndexInner + CA_SIZE].d) {
                        self.characters -> data[characterIndexInner + CA_XPOS].d = self.characters -> data[characterIndex + CA_XPOS].d + sin(theta / 57.2958) * combinedSize * 1.1;
                        self.characters -> data[characterIndexInner + CA_YPOS].d = self.characters -> data[characterIndex + CA_YPOS].d + cos(theta / 57.2958) * combinedSize * 1.1;
                    } else {
                        self.characters -> data[characterIndex + CA_XPOS].d = self.characters -> data[characterIndexInner + CA_XPOS].d + sin(theta / 57.2958) * combinedSize * 1.1;
                        self.characters -> data[characterIndex + CA_YPOS].d = self.characters -> data[characterIndexInner + CA_YPOS].d + cos(theta / 57.2958) * combinedSize * 1.1;
                    }
                }
            }
        }
    }
}

void render() {
    self.mouseHover = -1;
    /* render connections - TODO */

    /* render characters */
    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
        double xpos = (self.characters -> data[characterIndex + CA_XPOS].d + self.screenX) * self.zoom;
        double ypos = (self.characters -> data[characterIndex + CA_YPOS].d + self.screenY) * self.zoom;
        double size = self.characters -> data[characterIndex + CA_SIZE].d * self.zoom;
        double distance = (turtle.mouseX - xpos) * (turtle.mouseX - xpos) + (turtle.mouseY - ypos) * (turtle.mouseY - ypos);
        if (distance < size * size / 4) {
            self.mouseHover = characterIndex;
        }
        if (characterIndex == self.mouseHover || characterIndex == self.mouseDragging) {
            turtlePenColor(255, 255, 255);
            turtlePenSize(size * 1.05);
            turtleGoto(xpos, ypos);
            turtlePenDown();
            turtlePenUp();
        }
        turtlePenColor(255, 169, 169);
        turtlePenSize(size);
        turtleGoto(xpos, ypos);
        turtlePenDown();
        turtlePenUp();
        turtlePenColor(0, 0, 0);
        double textLength = turtleTextGetStringLength(self.characters -> data[characterIndex + CA_NAME].s, size);
        double factor = -0.48493 * log(size / textLength + 1) + 0.99230; // numbers calculated from 0.95 factor for Samburu Warrior, and 0.75 factor for Bix. Using log(size / textLength + 1)x + y
        double textSize = size / textLength * size * factor;
        turtleTextWriteString(self.characters -> data[characterIndex + CA_NAME].s, xpos, ypos, textSize, 50);
    }
}

void mouse() {
    if (turtleMouseDown()) {
        if (self.keys[KEYS_LMB] == 0) {
            /* first tick */
            self.keys[KEYS_LMB] = 1;
            if (self.mouseHover != -1) {
                self.mouseDragging = self.mouseHover;
                self.anchorX = self.characters -> data[self.mouseDragging + CA_XPOS].d;
                self.anchorY = self.characters -> data[self.mouseDragging + CA_YPOS].d;
                self.anchorMouseX = turtle.mouseX;
                self.anchorMouseY = turtle.mouseY;
            } else {
                self.anchorX = self.screenX;
                self.anchorY = self.screenY;
                self.anchorMouseX = turtle.mouseX;
                self.anchorMouseY = turtle.mouseY;
            }
        } else {
            if (self.mouseDragging != -1) {
                self.characters -> data[self.mouseDragging + CA_XPOS].d = self.anchorX + (turtle.mouseX - self.anchorMouseX) / self.zoom;
                self.characters -> data[self.mouseDragging + CA_YPOS].d = self.anchorY + (turtle.mouseY - self.anchorMouseY) / self.zoom;
            } else {
                self.screenX = self.anchorX + (turtle.mouseX - self.anchorMouseX) / self.zoom;
                self.screenY = self.anchorY + (turtle.mouseY - self.anchorMouseY) / self.zoom;
            }
        }
    } else {
        if (self.keys[KEYS_LMB]) {
            self.keys[KEYS_LMB] = 0;
            self.mouseDragging = -1;
        }
    }
    double scroll = turtleMouseWheel();
    if (scroll > 0) {
        self.screenX -= (turtle.mouseX * (-1 / self.scrollSpeed + 1)) / self.zoom;
        self.screenY -= (turtle.mouseY * (-1 / self.scrollSpeed + 1)) / self.zoom;
        self.zoom *= self.scrollSpeed;
    } else if (scroll < 0) {
        self.zoom /= self.scrollSpeed;
        self.screenX += (turtle.mouseX * (-1 / self.scrollSpeed + 1)) / self.zoom;
        self.screenY += (turtle.mouseY * (-1 / self.scrollSpeed + 1)) / self.zoom;
    }
    if (turtleKeyPressed(GLFW_KEY_SPACE)) {
        if (self.keys[KEYS_SPACE] == 0) {
            self.keys[KEYS_SPACE] = 1;
            removeOverlap();
        }
    } else {
        self.keys[KEYS_SPACE] = 0;
    }
}

int main(int argc, char *argv[]) {
    /* create window */
    GLFWwindow *window = turtleCreateWindowIcon(TURTLE_WINDOW_DEFAULT_WIDTH, TURTLE_WINDOW_DEFAULT_HEIGHT, "turtle demo", "images/thumbnail.png");
    if (window == NULL) {
        return -1; // failed to create window
    }

    /* initialise turtle */
    turtleSetResizeMode(TURTLE_RESIZE_MODE_PAD); // change to TURTLE_RESIZE_MODE_STRETCH to have content stretch when resized
    turtleInit(window, -320, -180, 320, 180);
    
    /* initialise osTools */
    osToolsInit(argv[0], window); // must include argv[0] to get executableFilepath, must include GLFW window for copy paste and cursor functionality
    osToolsFileDialogAddGlobalExtension("txt"); // add txt to extension restrictions
    osToolsFileDialogAddGlobalExtension("csv"); // add csv to extension restrictions

    /* initialise turtleText */
    char *constructedFilepath = malloc(5120);
    strcpy(constructedFilepath, osToolsFileDialog.executableFilepath);
    strcat(constructedFilepath, "config/roberto.tgl");
    turtleTextInit(constructedFilepath);
    free(constructedFilepath);

    turtleToolsSetTheme(TT_THEME_DARK);

    init();

    uint32_t tps = 120; // ticks per second (locked to fps in this case)
    clock_t start, end;
    
    while (turtle.close == 0) {
        start = clock();
        turtleGetMouseCoordinates();
        turtleClear();
        render();
        mouse();
        turtleToolsUpdate(); // update turtleTools
        tt_setColor(TT_COLOR_TEXT);
        turtleTextWriteStringf(-310, -170, 5, 0, "%.2lf, %.2lf", turtle.mouseX, turtle.mouseY);
        turtleUpdate(); // update the screen
        end = clock();
        while ((double) (end - start) / CLOCKS_PER_SEC < (1.0 / tps)) {
            end = clock();
        }
    }
    turtleFree();
    glfwTerminate();
    return 0;
}
