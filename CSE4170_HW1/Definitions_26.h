#pragma once

// 배경 색깔
// 대기 모드: WHITE, 다각형 생성 모드: BEIGE, 다각형 선택 모드: PALE BLUE LILY, 애니메이션 모드: MAGIC MINT
#define BACKGROUND_COLOR  1.0f, 1.0f, 1.0f
#define BACKGROUND_CREATE_COLOR 245.0f / 255.0f, 245.0f / 255.0f, 220.0f / 255.0f
#define BACKGROUND_SELECT_COLOR 207.0f / 255.0f, 236.0f / 255.0f, 236.0f / 255.0f
#define BACKGROUND_ANIMATION_COLOR 170.0f / 255.0f, 240.0f / 255.0f, 209.0f / 255.0f

#define POINT_COLOR  0.0f, 0.0f, 0.0f
#define LINE_COLOR_SELECTED  0.0f, 0.0f, 0.0f
#define CENTER_POINT_COLOR 0.0f, 0.0f, 0.0f  // 다각형 무게 중심 색깔
#define CENTER_POINT_COLOR_SELECTED 1.0f, 0.0f, 0.0f  // 다각형 선택 시 무게 중심 색깔

#define ROTATION_SENSITIVITY 200.0f
#define ROTATION_STEP 100 // ms
#define TRANSLATION_OFFSET 0.05f

// 더 빠르게 확대/축소될 수 있도록 조정 1.02f -> 1.03f
#define SCALE_UP_FACTOR		1.03f
#define SCALE_DOWN_FACTOR   (1.0f / SCALE_UP_FACTOR)

#define COS_5_DEGREES 0.9961947f
#define SIN_5_DEGREES 0.08715574f

#define CENTER_SELECTION_SENSITIVITY 0.01f

#define MAX_NUMBER_OF_POLYGONS 10

typedef struct {
	int width, height;
	int initial_anchor_x, initial_anchor_y;
} Window;

// 모드 전환(키보드와 마우스)
typedef struct {
	int standby_mode;  // 대기 모드
	int create_mode;  // 다각형 생성 모드: s
	int select_mode;  // 다각형 선택 모드: 무게 중심에 좌클릭
	int move_mode;  // 이동 모드, 다각형 선택 모드를 통해서만 접근 가능
	int rotation_mode;  // 회전 모드, 다각형 선택 모드를 통해서만 접근 가능
	int animation_mode;  // 애니메이션 모드: a
	int leftbuttonpressed;  // 마우스 좌클릭 상태
	int rightbuttonpressed;  // 마우스 우클릭 상태
} Status;

// 다각형 객체
#define MAX_NUMBER_OF_VERTICES_PER_POLYGON 128  // 최대 점 개수 128개
typedef struct {
	float point[MAX_NUMBER_OF_VERTICES_PER_POLYGON][2];
	int n_points;
	float center_x, center_y;
	int animation_val;
} My_Polygon;

// 2D 아핀 변환을 위한 행렬 구조체 정의
struct Mat3x3 {
	float mat3[3][3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
	};
};

// Functions for My_Polygon
void add_point(My_Polygon* pg, Window* wd, int x, int y);
void draw_lines_by_points(My_Polygon* pg, float line_color_r, float line_color_g, float line_color_b);
void update_center_of_gravity(My_Polygon* pg);

// void move_points(My_Polygon* pg, float del_x, float del_y);
// void rotate_points_around_center_of_grivity(My_Polygon* pg);
// void scale_points_around_center_of_grivity(My_Polygon* pg, float scale_factor);

// 2D 아핀 계산을 통한 기하 변환을 위해 새로 정의한 함수들
void affine_move(Mat3x3* M, float t_x, float t_y);
void affine_scale(Mat3x3* M, float s_x, float s_y);
void affine_rotate(Mat3x3* M, float theta);
Mat3x3 affine_combine(Mat3x3 M1, Mat3x3 M2);
My_Polygon* affine_vec_cal(My_Polygon* pg, Mat3x3* M);
void affine_move_polygon(My_Polygon* pg, float del_x, float del_y);
void affine_rotate_gravity_point(My_Polygon* pg, float theta);
void affine_scale_gravity_point(My_Polygon* pg, float f1, float f2);
void affine_scale_and_rotate_gravity_point(My_Polygon* pg, float f1, float f2, float theta);
