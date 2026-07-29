/*
Created by Ryan Srichai, 28.07.26

TODO:
- Save and load positions
- Hover popup for characters and connections
- Algorithm to determine position of nodes (possibly multiple algorithms)
- Finish all characters and connections
- Search function
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
    CA_RED = 10, // int
    CA_GREEN = 11, // int
    CA_BLUE = 12, // int
    CA_GROUPS = 13, // int, bitfield
    CA_HIGHLIGHTED = 14, // int
    CA_SELECTED = 15, // int
    CA_NUMBER_OF_FIELDS = 16,
};

enum {
    CO_NAME = 0, // only first name (string)
    CO_INDEX = 1, // int
    CO_DESCRIPTION = 2, // string
    CO_NUMBER_OF_FIELDS = 3,
};

enum {
    TH_INDEX = 0,
    TH_QUANTITY = 1,
    TH_NUMBER_OF_FIELDS = 2,
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

char groups[][64] = {
    "Flaming Dildos",
    "East Seventh Street",
    "Crandale Country Club",
    "Lou's Family",
    "Ted's Family",
    "Columbia Group",
    "Safari",
    "SweetSpot Networks",
    "Bennie's Family",
    "Country X",
    "No Group",
};

enum {
    GROUP_FLAMING_DILDOS = 0,
    GROUP_EAST_SEVENTH_STREET = 1,
    GROUP_CRANDALE_COUNTRY_CLUB = 2,
    GROUP_LOUS_FAMILY = 3,
    GROUP_TEDS_FAMILY = 4,
    GROUP_COLUMBIA_GROUP = 5,
    GROUP_SAFARI = 6,
    GROUP_SWEETSPOT_NETWORKS = 7,
    GROUP_BENNIES_FAMILY = 8,
    GROUP_COUNTRY_X = 9,
    GROUP_NO_GROUP = 10,
};

/* TODO - try using the colors from the cover of the book */
int32_t groupColors[] = {
    227, 99, 4, // Flaming Dildos
    127, 103, 82, // East Seventh Street
    88, 159, 76, // Crandale Country Club
    255, 171, 0, // Lou's Family
    150, 70, 51, // Ted's Family
    65, 129, 112, // Columbia Group
    121, 134, 102, // Safari
    16, 37, 116, // SweetSpot Networks
    236, 16, 236, // Bennie's Family
    38, 164, 186, // Country X
    255, 169, 169, // No Group
};

enum {
    KEYS_LMB,
    KEYS_RMB,
    KEYS_SPACE,
};

typedef struct {
    int8_t keys[32];

    list_t *characters; // CA_X

    /* screen pan and zoom */
    double screenX;
    double screenY;
    double zoom;
    double scrollSpeed;

    /* character and screen dragging */
    int32_t mouseHover; // index of self.characters of hovering character
    int32_t mouseDragging; // index of self.characters of dragging character
    double anchorX;
    double anchorY;
    double anchorMouseX;
    double anchorMouseY;

    /* highlight and selecting */
    int32_t highlighted; // index of self.characters of highlighted character
    int32_t selecting;
    double selectX;
    double selectY;
    double selectedChangeX;
    double selectedChangeY;
    int8_t selectRelease;

    /* sidebar */
    double sidebarX;
    int32_t hoverHistogram; // index of typeHistogram of group being hovered
    list_t *typeHistogram; // TH_X
    int32_t sumQuantity; // total number of group allocations (differs from total number of characters since characters can be in multiple groups)
} squad_t;

squad_t self;

void init() {
    self.characters = list_init();

    /* screen pan and zoom */
    self.screenX = 0;
    self.screenY = 0;
    self.zoom = 0.23;
    self.scrollSpeed = 1.15;

    /* character and screen dragging */
    self.mouseHover = -1;
    self.mouseDragging = -2;

    /* highlight and selecting */
    self.highlighted = -1;
    self.selecting = 0;
    self.selectedChangeX = 0;
    self.selectedChangeY = 0;
    self.selectRelease = 0;

    /* sidebar */
    self.sidebarX = 230;
    self.hoverHistogram = -1;
    self.sumQuantity = 0;
    self.typeHistogram = list_init();
    for (int32_t group = 0; group < sizeof(groups) / sizeof(groups[0]); group++) {
        list_append(self.typeHistogram, (unitype) group, 'i'); // TH_INDEX
        list_append(self.typeHistogram, (unitype) 0, 'i'); // TH_QUANTITY
    }

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
    if (strlen(firstName) == 1) {
        /* the M exception */
        firstName[1] = firstName[0];
        firstName[0] = ' ';
        firstName[2] = ' ';
        firstName[3] = '\0';
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
    list_append(self.characters, (unitype) randomDouble(-30, 30), 'd'); // CA_XPOS
    list_append(self.characters, (unitype) randomDouble(-30, 30), 'd'); // CA_YPOS
    list_append(self.characters, (unitype) 10.0, 'd'); // CA_SIZE
    list_append(self.characters, (unitype) groupColors[GROUP_NO_GROUP * 3 + 0], 'i'); // CA_RED
    list_append(self.characters, (unitype) groupColors[GROUP_NO_GROUP * 3 + 1], 'i'); // CA_GREEN
    list_append(self.characters, (unitype) groupColors[GROUP_NO_GROUP * 3 + 2], 'i'); // CA_BLUE
    list_append(self.characters, (unitype) 0, 'i'); // CA_GROUPS
    list_append(self.characters, (unitype) 0, 'i'); // CA_HIGHLIGHTED
    list_append(self.characters, (unitype) 0, 'i'); // CA_SELECTED
    /* read file */
    char *lineBuffer = malloc(4096);
    char *description = malloc(8192);
    description[0] = '\0';
    int32_t mode = 0;
    int32_t numberOfGroups = 0;
    while (fgets(lineBuffer, 4096, fp) != NULL) {
        while (strlen(lineBuffer) > 0 && (lineBuffer[strlen(lineBuffer) - 1] == '\r' || lineBuffer[strlen(lineBuffer) - 1] == '\n')) {
            lineBuffer[strlen(lineBuffer) - 1] = '\0';
        }
        if (streq(lineBuffer, "# Connections")) {
            free(self.characters -> data[characterIndex + CA_DESCRIPTION].s);
            self.characters -> data[characterIndex + CA_DESCRIPTION].s = description;
            mode = 1;
            if (numberOfGroups == 0) {
                self.characters -> data[characterIndex + CA_GROUPS].i |= 1 << GROUP_NO_GROUP;
                self.typeHistogram -> data[GROUP_NO_GROUP * TH_NUMBER_OF_FIELDS + TH_QUANTITY].i++;
                self.sumQuantity++;
            } else {
                self.sumQuantity += numberOfGroups;
            }
            continue;
        }
        if (mode == 0) {
            if (strncmp(lineBuffer, "Born: ", strlen("Born: ")) == 0) {
                /* birthday */
                char *line = lineBuffer + strlen("Born: ");
                int32_t foundMonth = 0;
                for (int32_t month = 0; month < 12; month++) {
                    if (strncmp(line, months[month], strlen(months[month]))) {
                        foundMonth = month + 1;
                        line += strlen(months[month]);
                        break;
                    }
                }
                if (foundMonth) { // TODO
                    if (0) {
                        /* month, day, and year */
                    } else {
                        /* just month and year */
                    }
                } else {
                    /* just year */
                    sscanf(line, "%d", &self.characters -> data[characterIndex + CA_BIRTHYEAR].i);
                }
            } else if (strncmp(lineBuffer, "Groups: ", strlen("Groups: ")) == 0) {
                /* groups */
                double red = 0;
                double green = 0;
                double blue = 0;
                char *line = lineBuffer + strlen("Groups: ");
                char *ptr = strtok(line, ",");
                while (ptr != NULL) {
                    int32_t groupAdd = -1;
                    for (int32_t group = 0; group < sizeof(groups) / sizeof(groups[0]); group++) {
                        if (strncmp(ptr, groups[group], strlen(groups[group])) == 0) {
                            groupAdd = group;
                            break;
                        }
                    }
                    if (groupAdd != -1) {
                        /* https://www.reddit.com/r/roguelikedev/comments/eyn7sr/how_to_mix_two_colour_lights */
                        red = min(sqrt(red * red + groupColors[groupAdd * 3 + 0] * groupColors[groupAdd * 3 + 0]), 255);
                        green = min(sqrt(green * green + groupColors[groupAdd * 3 + 1] * groupColors[groupAdd * 3 + 1]), 255);
                        blue = min(sqrt(blue * blue + groupColors[groupAdd * 3 + 2] * groupColors[groupAdd * 3 + 2]), 255);
                        self.characters -> data[characterIndex + CA_GROUPS].i |= 1 << groupAdd;
                        self.typeHistogram -> data[groupAdd * TH_NUMBER_OF_FIELDS + TH_QUANTITY].i++;
                        numberOfGroups++;
                    }
                    ptr = strtok(NULL, ",");
                    if (ptr != NULL) {
                        ptr++; // skip space
                    }
                }
                if (numberOfGroups > 0) {
                    self.characters -> data[characterIndex + CA_RED].i = red;
                    self.characters -> data[characterIndex + CA_GREEN].i = green;
                    self.characters -> data[characterIndex + CA_BLUE].i = blue;
                }
            } else if (strncmp(lineBuffer, "Mentioned: ", strlen("Mentioned: "))) {
                /* mentioned */
                sscanf(lineBuffer + strlen("Mentioned "), "%d", &self.characters -> data[characterIndex + CA_MENTIONED].i);
                self.characters -> data[characterIndex + CA_SIZE].d = log(self.characters -> data[characterIndex + CA_MENTIONED].i + 1) * 4 + 10;
            }
            /* description */
            strcat(description, lineBuffer);
            strcat(description, "\n");
        } else {
            /* connection */
            char *line = lineBuffer + strlen("- "); // skip "- "
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
            } else {
                list_append(connections, (unitype) line, 's'); // CO_NAME
                list_append(connections, (unitype) -1, 'i'); // CO_INDEX
                list_append(connections, (unitype) "NULL", 's'); // CO_DESCRIPTION
            }
        }
    }
    self.characters -> data[characterIndex + CA_SIZE].d += connections -> length / CO_NUMBER_OF_FIELDS * 10;
    fclose(fp);
    free(lineBuffer);
    return 0;
}

void removeOverlap() {
    /* repeat 15 times */
    for (int32_t i = 0; i < 15; i++) {
        for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
            /* sweep five times */
            for (int32_t j = 0; j < 5; j++) {
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
}

void render() {
    /* init */
    self.mouseHover = -1;
    double leftThresh = 0;
    double rightThresh = 0;
    double upThresh = 0;
    double downThresh = 0;
    if (self.selecting) {
        if (self.selectX > turtle.mouseX) {
            leftThresh = turtle.mouseX;
            rightThresh = self.selectX;
        } else {
            rightThresh = turtle.mouseX;
            leftThresh = self.selectX;
        }
        if (self.selectY > turtle.mouseY) {
            downThresh = turtle.mouseY;
            upThresh = self.selectY;
        } else {
            upThresh = turtle.mouseY;
            downThresh = self.selectY;
        }
    }
    /* translate selected characters */
    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
        if (self.characters -> data[characterIndex + CA_SELECTED].i >= 2) {
            if (self.selectRelease) {
                if (self.characters -> data[characterIndex + CA_HIGHLIGHTED].i == 0) {
                    self.characters -> data[characterIndex + CA_SELECTED].i = 0;
                }
            } else if (self.mouseDragging != characterIndex) {
                self.characters -> data[characterIndex + CA_XPOS].d += self.selectedChangeX;
                self.characters -> data[characterIndex + CA_YPOS].d += self.selectedChangeY;
            }
        }
    }
    /* render connections */
    turtlePenShape("none");
    turtlePenSize(self.zoom * 1.2);
    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
        double xpos = (self.characters -> data[characterIndex + CA_XPOS].d + self.screenX) * self.zoom;
        double ypos = (self.characters -> data[characterIndex + CA_YPOS].d + self.screenY) * self.zoom;
        turtlePenColorAlpha(self.characters -> data[characterIndex + CA_RED].i, self.characters -> data[characterIndex + CA_GREEN].i, self.characters -> data[characterIndex + CA_BLUE].i, 230);
        for (int32_t connectionIndex = 0; connectionIndex < self.characters -> data[characterIndex + CA_CONNECTIONS].r -> length; connectionIndex += CO_NUMBER_OF_FIELDS) {
            int32_t index = self.characters -> data[characterIndex + CA_CONNECTIONS].r -> data[connectionIndex + CO_INDEX].i;
            if (index == -1) {
                continue;
            }
            double cx = (self.characters -> data[index + CA_XPOS].d + self.screenX) * self.zoom;
            double cy = (self.characters -> data[index + CA_YPOS].d + self.screenY) * self.zoom;
            turtleGoto(xpos, ypos);
            turtlePenDown();
            turtleGoto(cx, cy);
            turtlePenUp();
        }
    }
    turtlePenShape("circle");
    /* render characters */
    if (self.mouseDragging < 0) {
        for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
            double xpos = (self.characters -> data[characterIndex + CA_XPOS].d + self.screenX) * self.zoom;
            double ypos = (self.characters -> data[characterIndex + CA_YPOS].d + self.screenY) * self.zoom;
            double size = self.characters -> data[characterIndex + CA_SIZE].d * self.zoom;
            if (xpos - size / 2 > 320 || xpos + size / 2 < -320 || ypos - size / 2 > 180 || ypos + size / 2 < -180) {
                continue;
            }
            double distance = (turtle.mouseX - xpos) * (turtle.mouseX - xpos) + (turtle.mouseY - ypos) * (turtle.mouseY - ypos);
            if (distance < size * size / 4) {
                self.mouseHover = characterIndex;
            }
        }
    }
    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
        double xpos = (self.characters -> data[characterIndex + CA_XPOS].d + self.screenX) * self.zoom;
        double ypos = (self.characters -> data[characterIndex + CA_YPOS].d + self.screenY) * self.zoom;
        double size = self.characters -> data[characterIndex + CA_SIZE].d * self.zoom;
        if (xpos - size / 2 > 320 || xpos + size / 2 < -320 || ypos - size / 2 > 180 || ypos + size / 2 < -180) {
            continue;
        }
        if (self.selecting) {
            if (xpos + size / 2 >= leftThresh && xpos - size / 2 <= rightThresh && ypos + size / 2 >= downThresh && ypos - size / 2 <= upThresh) {
                self.characters -> data[characterIndex + CA_SELECTED].i = 1;
            } else {
                if (self.characters -> data[characterIndex + CA_SELECTED].i == 1) {
                    self.characters -> data[characterIndex + CA_SELECTED].i = 0;
                }
            }
        } else {
            if (self.characters -> data[characterIndex + CA_SELECTED].i == 1) {
                self.characters -> data[characterIndex + CA_SELECTED].i = 2;
            }
        }
        if (characterIndex == self.mouseHover || characterIndex == self.mouseDragging) {
            tt_setColor(TT_COLOR_WHITE);
            turtlePenSize(size * 1.05);
            turtleGoto(xpos, ypos);
            turtlePenDown();
            turtlePenUp();
        }
        if (self.characters -> data[characterIndex + CA_SELECTED].i && self.characters -> data[characterIndex + CA_SELECTED].i != 3) {
            turtlePenColor(120, 120, 120);
        } else {
            turtlePenColor(self.characters -> data[characterIndex + CA_RED].i, self.characters -> data[characterIndex + CA_GREEN].i, self.characters -> data[characterIndex + CA_BLUE].i);
        }
        turtlePenSize(size);
        if (self.highlighted != -1) {
            if (self.characters -> data[characterIndex + CA_HIGHLIGHTED].i == 0 && self.highlighted != characterIndex) {
                turtle.pena = 0.25; // 25% opacity
            }
        }
        if (self.hoverHistogram != -1) {
            if (!(self.characters -> data[characterIndex + CA_GROUPS].i & (1 << self.typeHistogram -> data[self.hoverHistogram].i))) {
                turtle.pena = 0.25; // 25% opacity
            }
        }
        turtleGoto(xpos, ypos);
        turtlePenDown();
        turtlePenUp();
        if (self.characters -> data[characterIndex + CA_RED].i + self.characters -> data[characterIndex + CA_GREEN].i + self.characters -> data[characterIndex + CA_BLUE].i < 150) {
            tt_setColor(TT_COLOR_WHITE);
        } else {
            tt_setColor(TT_COLOR_BLACK);
        }
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
            if (turtle.mouseX < self.sidebarX) {
                self.anchorMouseX = turtle.mouseX;
                self.anchorMouseY = turtle.mouseY;
                if (turtleKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
                    /* selecting */
                    self.selectX = turtle.mouseX;
                    self.selectY = turtle.mouseY;
                    self.selecting = 1;
                } else {
                    if (self.mouseHover != -1) {
                        self.mouseDragging = self.mouseHover;
                        if (self.characters -> data[self.mouseDragging + CA_SELECTED].i == 0) {
                            self.selectRelease = 1;
                        }
                        self.anchorX = self.characters -> data[self.mouseDragging + CA_XPOS].d;
                        self.anchorY = self.characters -> data[self.mouseDragging + CA_YPOS].d;
                    } else {
                        self.selectRelease = 1;
                        self.mouseDragging = -1;
                        self.anchorX = self.screenX;
                        self.anchorY = self.screenY;
                    }
                }
            } else {
                if (self.hoverHistogram != -1) {
                    /* unhighlight every node */
                    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
                        self.characters -> data[characterIndex + CA_HIGHLIGHTED].i = 0;
                        self.characters -> data[characterIndex + CA_SELECTED].i = 0;
                    }
                    /* highlight hoverHistogram type */
                    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
                        if (self.characters -> data[characterIndex + CA_GROUPS].i & (1 << self.typeHistogram -> data[self.hoverHistogram].i)) {
                            self.characters -> data[characterIndex + CA_HIGHLIGHTED].i = 1;
                            self.characters -> data[characterIndex + CA_SELECTED].i = 3;
                            self.highlighted = characterIndex;
                        }
                    }
                }
            }
        } else {
            /* mouse held */
            if (self.selecting == 1) {
                /* render selection box */
                turtleGoto(self.selectX, self.selectY);
                turtlePenSize(2 * self.zoom);
                turtlePenColor(120, 120, 120);
                turtlePenDown();
                turtleGoto(turtle.mouseX, self.selectY);
                turtleGoto(turtle.mouseX, turtle.mouseY);
                turtleGoto(self.selectX, turtle.mouseY);
                turtleGoto(self.selectX, self.selectY);
                turtlePenUp();
            } else {
                if (self.mouseDragging >= 0) {
                    self.selectedChangeX = self.anchorX + (turtle.mouseX - self.anchorMouseX) / self.zoom - self.characters -> data[self.mouseDragging + CA_XPOS].d;
                    self.selectedChangeY = self.anchorY + (turtle.mouseY - self.anchorMouseY) / self.zoom - self.characters -> data[self.mouseDragging + CA_YPOS].d;
                    self.characters -> data[self.mouseDragging + CA_XPOS].d = self.anchorX + (turtle.mouseX - self.anchorMouseX) / self.zoom;
                    self.characters -> data[self.mouseDragging + CA_YPOS].d = self.anchorY + (turtle.mouseY - self.anchorMouseY) / self.zoom;
                } else if (self.mouseDragging == -1) {
                    self.screenX = self.anchorX + (turtle.mouseX - self.anchorMouseX) / self.zoom;
                    self.screenY = self.anchorY + (turtle.mouseY - self.anchorMouseY) / self.zoom;
                }
            }
        }
    } else {
        if (self.keys[KEYS_LMB]) {
            self.keys[KEYS_LMB] = 0;
            if (fabs(turtle.mouseX - self.anchorMouseX) < 1 && fabs(turtle.mouseY - self.anchorMouseY) < 1) {
                if (self.mouseDragging < 0) {
                    if (self.selecting == 0) {
                        /* unhighlight every node */
                        for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
                            self.characters -> data[characterIndex + CA_HIGHLIGHTED].i = 0;
                            self.characters -> data[characterIndex + CA_SELECTED].i = 0;
                        }
                        self.highlighted = -1;
                    }
                } else {
                    /* unhighlight every node */
                    for (int32_t characterIndex = 0; characterIndex < self.characters -> length; characterIndex += CA_NUMBER_OF_FIELDS) {
                        self.characters -> data[characterIndex + CA_HIGHLIGHTED].i = 0;
                        self.characters -> data[characterIndex + CA_SELECTED].i = 0;
                    }
                    /* highlight connections of highlighted */
                    self.highlighted = self.mouseDragging;
                    self.characters -> data[self.highlighted + CA_HIGHLIGHTED].i = 1;
                    self.characters -> data[self.highlighted + CA_SELECTED].i = 3;
                    for (int32_t connectionIndex = 0; connectionIndex < self.characters -> data[self.highlighted + CA_CONNECTIONS].r -> length; connectionIndex += CO_NUMBER_OF_FIELDS) {
                        int32_t character = self.characters -> data[self.highlighted + CA_CONNECTIONS].r -> data[connectionIndex + CO_INDEX].i;
                        self.characters -> data[character + CA_HIGHLIGHTED].i = 1;
                        self.characters -> data[character + CA_SELECTED].i = 3;
                    }
                }
            }
            self.mouseDragging = -2;
            self.selecting = 0;
            self.selectRelease = 0;
            self.selectedChangeX = 0;
            self.selectedChangeY = 0;
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

void sidebar() {
    /* draw sidebar */
    turtleRectangleColor(self.sidebarX, -180, 320, 180, 0, 0, 0, 50);
    double legendWidth = 80;
    double legendHeight = 200;
    double legendX = (self.sidebarX + 320) / 2 - legendWidth / 2;
    double legendY = 160;
    double ypos = legendY;
    self.hoverHistogram = -1;
    for (int32_t type = 0; type < self.typeHistogram -> length; type += TH_NUMBER_OF_FIELDS) {
        double saveY = ypos;
        ypos -= ((double) (self.typeHistogram -> data[type + TH_QUANTITY].i)) / self.sumQuantity * legendHeight;
        int32_t color = self.typeHistogram -> data[type + TH_INDEX].i * 3;
        turtlePenColor(groupColors[color], groupColors[color + 1], groupColors[color + 2]);
        turtleRectangle(legendX, saveY, legendX + legendWidth, ypos);
        if (turtle.mouseX >= legendX && turtle.mouseX <= legendX + legendWidth && turtle.mouseY >= ypos && turtle.mouseY < saveY) {
            turtlePenColor(255, 255, 255);
            turtleTriangle(legendX - 10, (saveY + ypos) / 2 + 5, legendX - 10, (saveY + ypos) / 2 - 5, legendX - 3, (saveY + ypos) / 2);
            turtleTextWriteStringf(legendX - 15, (saveY + ypos) / 2, 8, 100, "%s (%d)", groups[self.typeHistogram -> data[type + TH_INDEX].i], self.typeHistogram -> data[type + TH_QUANTITY].i);
            self.hoverHistogram = type;
        }
        if (saveY - ypos > 6) {
            turtlePenColor(0, 0, 0);
            turtleTextWriteString(groups[self.typeHistogram -> data[type + TH_INDEX].i], legendX + legendWidth / 2, (saveY + ypos) / 2, 5, 50);
        }
    }
}

int main(int argc, char *argv[]) {
    /* create window */
    GLFWwindow *window = turtleCreateWindowIcon(TURTLE_WINDOW_DEFAULT_WIDTH, TURTLE_WINDOW_DEFAULT_HEIGHT, "Gooniverse", "images/thumbnail.png");
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
        sidebar();
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
