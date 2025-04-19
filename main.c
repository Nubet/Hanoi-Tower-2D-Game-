#include "primlib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_NUMBER_OF_RODS 10
#define NUMBER_OF_RODS 9
#define NUMBER_OF_DISKS 20
#define MIN_NUMBER_OF_DISKS 3
#define MAX_NUMBER_OF_DISKS 40
#define MIN_ROD_HEIGHT 300
#define SCREEN_WIDTH gfx_screenWidth()
#define SCREEN_HEIGHT gfx_screenHeight()

static int max_disk_width;
const int min_disk_width = 30;
const int animation_step = 5;

typedef struct {
    int collection[NUMBER_OF_DISKS];
    int capacity;
    int size;
    int rod_number;
} Stack;

typedef struct {
    int current_x;
    int current_y;
    int target_x;
    int target_y;
    int disk;
} AnimationState;

/* STACK  */
void init_stack(Stack* stack, int capacity, int rod_number);
void destroy_stack(Stack* stack);
bool is_full(Stack* stack);
bool is_empty(Stack* stack);
bool pop(Stack* stack, int* item);
bool push(Stack* stack, int item);
bool top(Stack* stack, int* item);
void print_stack(Stack* stack);

/* GAME LOGIC */
bool win(Stack *stack);
void display_win_message();
bool is_enough_space_in_rod(Stack* origin, Stack* destination);
bool is_move_valid(Stack* origin, Stack* destination);
void perform_move(Stack* origin, Stack* destination);
bool move_disk(Stack* origin_rod, Stack* destination_rod, Stack rods[]);

/* DRAWING */
int scale_disk_height(int base_y);
void calculate_rod_x_pos(int base_y, int rods_x_position[]);
enum color get_disk_color(int disk_size);
int map_value_to_new_range(int min_disk_width, int max_disk_width, int value_to_map);
void draw_disks(Stack* rod, int rod_center_x, int base_y);
void draw_base(int base_y);
void draw_disk_number_above_the_rod(int x, int y, int index);
void draw_single_rod(int rod_x1, int rod_top_y, int rod_x2, int rod_base_y, enum color c);
void draw_rods_and_disk(int base_y, int rods_x_position[], Stack rods[]);
void redraw_scene(Stack rods[]);
void redraw_scene_with_disk(int moving_disk_x, int moving_disk_y, int disk, Stack rods[]);

/* ANIMATIONS */
void animate_move(int rod_origin_x, int rod_destination_x, int start_y, int end_y, int disk, Stack rods[]);
void animate_lift_disk(int* current_x, int* current_y, int target_y, int disk, Stack rods[]);
void animate_move_horizontal(int* current_x, int target_x, int current_y, int disk, Stack rods[]);
void animate_lower_disk(int* current_y, int target_y, int current_x, int disk, Stack rods[]);

/* GAME CONTROL */
void init_game(Stack rods[]);
void reset_rod_selection(int *origin_rod, int *destination_rod);
bool is_valid_rod_key(int key);
void process_special_keys(int key, int* origin, int* dest);
void handle_rod_selection(int key, int* origin, int* dest, Stack rods[]);
void handle_input_loop(Stack rods[]);

int main() {
    if (gfx_init()) {
        exit(3);
    }
    Stack rods[MAX_NUMBER_OF_RODS];
    init_game(rods);
    handle_input_loop(rods);
    return 0;
}

/* STACK FUNCTIONS */
void init_stack(Stack* stack, int capacity, int rod_number) {
    if (capacity <= 0) {
        printf("s: stack capacity must be > 0!\n");
        return;
    }
    stack->capacity = capacity;
    stack->size = 0;
    stack->rod_number = rod_number;
}

bool is_full(Stack* stack) {
    return stack->capacity == stack->size;
}

bool is_empty(Stack* stack) {
    return stack->size == 0;
}

bool push(Stack* stack, int item) {
    if (is_full(stack)) {
        printf("s: can't push item into a full stack\n");
        return false;
    }
    stack->collection[stack->size] = item;
    stack->size++;
    printf("s: pushing item: %d\n", item);
    return true;
}

bool top(Stack* stack, int* item) {
    if (is_empty(stack)) {
        printf("s: can't top item because stack is empty\n");
        return false;
    }
    *item = stack->collection[stack->size - 1];
    return true;
}

bool pop(Stack* stack, int* item) {
    if (is_empty(stack)) {
        printf("s: can't pop item because stack is empty\n");
        return false;
    }
    stack->size--;
    *item = stack->collection[stack->size];
    printf("s: item %d has been popped from stack\n", *item);
    return true;
}

void print_stack(Stack* stack) {
    if (stack == NULL) {
        printf("s: eror stack pointer is NULL\n");
        return;
    }
    printf("Rod %d: ", stack->rod_number);
    if (is_empty(stack)) {
        printf("s: Stack is empty.\n");
        return;
    }
    for (int i = 0; i < stack->size; i++) {
        printf("%d ", stack->collection[i]);
    }
    printf("\n");
}

/* GAME LOGIC FUNCTIONS */
void display_win_message() {
    gfx_filledRect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BLACK);
    gfx_textout((SCREEN_WIDTH - 80)/2,SCREEN_HEIGHT/2,"You have won!",BLUE);
    gfx_textout((SCREEN_WIDTH - 130)/2,(SCREEN_HEIGHT/2)+20,"Press ANY KEY to exit", WHITE);
    gfx_updateScreen();

    gfx_getkey();
    exit(0);
}

bool win(Stack *stack) {
    if (stack->size != NUMBER_OF_DISKS) {
        return false;
    }
    for (int i = 0; i < stack->size; i++) {
        if (stack->collection[i] != stack->size - i) {
            return false;
        }
    }
    return true;
}

bool is_enough_space_in_rod(Stack* origin, Stack* destination) {
    if (origin->rod_number == destination->rod_number) {
        printf("Cannot move to the same rod\n");
        return false;
    }
    if (is_empty(origin)) {
        printf("Origin rod is empty\n");
        return false;
    }
    if (is_full(destination)) {
        printf("Destination rod is full\n");
        return false;
    }
    return true;
}

bool is_move_valid(Stack* origin, Stack* destination) {
    int top_origin, top_destination;
    top(origin, &top_origin);
    if (is_empty(destination))
        return true;
    top(destination, &top_destination);
    return top_origin < top_destination;
}

void perform_move(Stack* origin, Stack* destination) {
    int disk;
    pop(origin, &disk);
    push(destination, disk);
}

bool move_disk(Stack* origin_rod, Stack* destination_rod, Stack rods[]) {
    if (!is_enough_space_in_rod(origin_rod, destination_rod))
        return false;
    if (!is_move_valid(origin_rod, destination_rod)) {
        printf("Invalid disk placement\n");
        return false;
    }

    int disk;
    pop(origin_rod, &disk);

    int base_y = SCREEN_HEIGHT - 50;
    int rods_x_position[NUMBER_OF_RODS];
    calculate_rod_x_pos(base_y, rods_x_position);
    int rod_origin_x = rods_x_position[origin_rod->rod_number - 1];
    int rod_destination_x = rods_x_position[destination_rod->rod_number - 1];

    int disk_height = scale_disk_height(base_y);
    int start_y = base_y - (origin_rod->size + 1) * disk_height;
    int end_y = base_y - (destination_rod->size + 1) * disk_height;
    animate_move(rod_origin_x, rod_destination_x, start_y, end_y, disk, rods);

    push(destination_rod, disk);
    printf("Moved disk from rod %d to %d\n", origin_rod->rod_number, destination_rod->rod_number);

    if (win(destination_rod) && destination_rod->rod_number == NUMBER_OF_RODS) {
        printf("You have won!\n");
        display_win_message();
    }

    return true;
}

/* DRAWING FUNCTIONS */
int scale_disk_height(int base_y) {
    const int top_margin = 50;
    return (float)(base_y - top_margin) / MAX_NUMBER_OF_DISKS;
}

void calculate_rod_x_pos(int base_y, int rods_x_position[]) {
    const int rod_spacing = SCREEN_WIDTH / (NUMBER_OF_RODS + 1);
    for (int i = 0; i < NUMBER_OF_RODS; i++) {
        rods_x_position[i] = rod_spacing * (i + 1);
    }
}

enum color get_disk_color(int disk_size) {
    enum color diskColor;
    switch (disk_size % 7) {
        case 0: diskColor = RED; break;
        case 1: diskColor = GREEN; break;
        case 2: diskColor = BLUE; break;
        case 3: diskColor = CYAN; break;
        case 4: diskColor = MAGENTA; break;
        case 5: diskColor = YELLOW; break;
        case 6: diskColor = WHITE; break;
        default: diskColor = RED; break;
    }
    return diskColor;
}

int map_value_to_new_range(int min_disk_width, int max_disk_width, int value_to_map) {
    int x_min = 0;
    int x_max = NUMBER_OF_DISKS;
    int length_of_original_range = x_max - x_min;
    int y_min = min_disk_width;
    int y_max = max_disk_width;
    int length_of_new_range = y_max - y_min;
    double proportion_in_original_range = (double)(value_to_map - x_min) / length_of_original_range;
    return y_min + ceil(length_of_new_range * proportion_in_original_range);
}

void draw_disks(Stack* rod, int rod_center_x, int base_y) {
    float disk_height = scale_disk_height(base_y);
    for (int disk_index = 0; disk_index < rod->size; disk_index++) {
        int current_disk_size = rod->collection[disk_index];
        int current_disk_width = map_value_to_new_range(min_disk_width, max_disk_width, current_disk_size);
        float disk_top_y = base_y - (disk_index + 1) * (disk_height);
        enum color disk_color = get_disk_color(current_disk_size);
        gfx_filledRect(rod_center_x - current_disk_width / 2,
                      (int) disk_top_y + 1,
                      rod_center_x + current_disk_width / 2,
                      (int) (disk_top_y + disk_height),
                      disk_color);
    }
}

void draw_base(int base_y) {
    const int base_height = 20;
    gfx_filledRect(0, base_y, SCREEN_WIDTH - 1, base_y + base_height, WHITE);
}

void draw_disk_number_above_the_rod(int x, int y, int index) {
    char number[3];
    sprintf(number, "%d", index + 1);
    gfx_textout(x, y, number, RED);
}

void draw_single_rod(int rod_x1, int rod_top_y, int rod_x2, int rod_base_y, enum color c) {
    gfx_filledRect(rod_x1, rod_top_y, rod_x2, rod_base_y, c);
}

void draw_rods_and_disk(int base_y, int rods_x_position[], Stack rods[]) {
    int rod_width = 20;
    int disk_height = scale_disk_height(base_y);
    int computed_rod_height = NUMBER_OF_DISKS * disk_height;
    int rod_height = (computed_rod_height < MIN_ROD_HEIGHT) ? MIN_ROD_HEIGHT : computed_rod_height;
    int rod_top = base_y - rod_height;

    for (int i = 0; i < NUMBER_OF_RODS; i++) {
        draw_single_rod(rods_x_position[i] - rod_width / 2, rod_top,
                       rods_x_position[i] + rod_width / 2, base_y, WHITE);
        draw_disk_number_above_the_rod(rods_x_position[i] - 5, rod_top - 20, i);
        draw_disks(&rods[i], rods_x_position[i], base_y);
    }
}

void redraw_scene(Stack rods[]) {
    gfx_filledRect(0, 0, gfx_screenWidth() - 1, gfx_screenHeight() - 1, BLACK);
    int base_y = SCREEN_HEIGHT - 50;
    draw_base(base_y);
    int rods_x_position[NUMBER_OF_RODS];
    calculate_rod_x_pos(base_y, rods_x_position);
    draw_rods_and_disk(base_y, rods_x_position, rods);
    gfx_updateScreen();
}

void redraw_scene_with_disk(int moving_disk_x, int moving_disk_y, int disk, Stack rods[]) {
    gfx_filledRect(0, 0, gfx_screenWidth() - 1, gfx_screenHeight() - 1, BLACK);
    int base_y = SCREEN_HEIGHT - 50;
    draw_base(base_y);
    int rods_x_position[NUMBER_OF_RODS];
    calculate_rod_x_pos(base_y, rods_x_position);
    draw_rods_and_disk(base_y, rods_x_position, rods);
    int disk_width = map_value_to_new_range(min_disk_width, max_disk_width, disk);
    int disk_height = scale_disk_height(base_y);
    enum color disk_color = get_disk_color(disk);
    gfx_filledRect(moving_disk_x - disk_width / 2, moving_disk_y,
                  moving_disk_x + disk_width / 2, moving_disk_y + disk_height,
                  disk_color);
    gfx_updateScreen();
}

void animate_lift_disk(int* current_x, int* current_y, int target_y, int disk, Stack rods[]) {
    while (*current_y > target_y) {
        *current_y -= animation_step;
        redraw_scene_with_disk(*current_x, *current_y, disk, rods);
        SDL_Delay(10);
    }
}

void animate_move_horizontal(int* current_x, int target_x, int current_y, int disk, Stack rods[]) {
    while (*current_x != target_x) {
        *current_x += (*current_x < target_x) ? animation_step : -animation_step;
        if (abs(*current_x - target_x) < animation_step) {
            *current_x = target_x;
        }
        redraw_scene_with_disk(*current_x, current_y, disk, rods);
        SDL_Delay(10);
    }
}

void animate_lower_disk(int* current_y, int target_y, int current_x, int disk, Stack rods[]) {
    while (*current_y < target_y) {
        *current_y += animation_step;
        redraw_scene_with_disk(current_x, *current_y, disk, rods);
        SDL_Delay(10);
    }
}

int calculate_lift_height(int start_y, int base_y, int rod_height, int disk_height) {
    int lift_y = base_y - rod_height - disk_height - 5;
    return (lift_y > start_y) ? start_y : lift_y;
}

void animate_move(int rod_origin_x, int rod_destination_x, int start_y, int end_y, int disk, Stack rods[]) {
    int base_y = SCREEN_HEIGHT - 50;
    int disk_height = scale_disk_height(base_y);
    int computed_rod_height = NUMBER_OF_DISKS * disk_height;
    int rod_height = (computed_rod_height < MIN_ROD_HEIGHT) ? MIN_ROD_HEIGHT : computed_rod_height;

    int current_x = rod_origin_x;
    int current_y = start_y;

    int lift_y = calculate_lift_height(start_y, base_y, rod_height, disk_height);
    animate_lift_disk(&current_x, &current_y, lift_y, disk, rods);
    animate_move_horizontal(&current_x, rod_destination_x, current_y, disk, rods);
    animate_lower_disk(&current_y, end_y, current_x, disk, rods);
}


/* GAME CONTROL FUNCTIONS */
void init_game(Stack rods[]) {
    max_disk_width = (SCREEN_WIDTH / (NUMBER_OF_RODS + 1)) - 20;
    if (NUMBER_OF_DISKS > MAX_NUMBER_OF_DISKS) {
        printf("Disks number is higher than rod capacity!\n");
        exit(-1);
    }
    else if (NUMBER_OF_DISKS < MIN_NUMBER_OF_DISKS){
        printf("Disks number is lower than required minimum!\n");
        exit(-1);
    }
    for (int i = 0; i < NUMBER_OF_RODS; i++) {
        init_stack(&rods[i], NUMBER_OF_DISKS, i + 1);
    }
    for (int i = NUMBER_OF_DISKS; i > 0; i--) {
        push(&rods[0], i);
    }
    redraw_scene(rods);
}

void reset_rod_selection(int *origin_rod, int *destination_rod) {
    *origin_rod = -1;
    *destination_rod = -1;
}

bool is_valid_rod_key(int key) {
    if (key == '0') {
        return NUMBER_OF_RODS == 10;
    }

    int rod = key - '0';
    return ((rod >= 1 && rod <= NUMBER_OF_RODS));
}

void process_special_keys(int key, int* origin_rod, int* dest) {
    switch (key) {
        case SDLK_ESCAPE:
            exit(0);
        case SDLK_r:
            reset_rod_selection(origin_rod, dest);
            break;
        default:
            break;
    }
}

void handle_rod_selection(int key, int* origin_rod, int* destination_rod, Stack rods[]) {
    int selected_rod = (key == '0') ? 9 : key - '0' - 1;
    if (*origin_rod == -1) {
        *origin_rod = selected_rod;
        printf("Selected origin rod: %d\n", selected_rod + 1);
    } else {
        *destination_rod = selected_rod;
        printf("Selected destination rod: %d\n", selected_rod + 1);
        if (move_disk(&rods[*origin_rod], &rods[*destination_rod], rods)) {
            redraw_scene(rods);
        }
        reset_rod_selection(origin_rod, destination_rod);
    }
}

void handle_input_loop(Stack rods[]) {
    int origin_rod = -1;
    int destination_rod = -1;
    while (1) {
        int key = gfx_pollkey();
        if (key != -1) {
            process_special_keys(key, &origin_rod, &destination_rod);
            if (is_valid_rod_key(key)) {
                handle_rod_selection(key, &origin_rod, &destination_rod, rods);
            }
            else {
                reset_rod_selection(&origin_rod,&destination_rod);
            }
        }
        SDL_Delay(10);
    }
}
