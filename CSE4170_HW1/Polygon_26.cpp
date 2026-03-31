#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "Definitions_26.h"

void add_point(My_Polygon *pg, Window *wd, int x, int y) {
	pg->point[pg->n_points][0] = 2.0f * ((float)x) / wd->width - 1.0f;
	pg->point[pg->n_points][1] = 2.0f * ((float)wd->height - y) / wd->height - 1.0f;
	pg->n_points++; 
}

void draw_lines_by_points(My_Polygon* pg, float line_color_r, float line_color_g, float line_color_b) {
	glColor3f(POINT_COLOR);  // 점 색
	for (int i = 0; i < pg->n_points; i++) {
		glBegin(GL_POINTS);
		glVertex2f(pg->point[i][0], pg->point[i][1]);
		glEnd();
	}
	glColor3f(line_color_r, line_color_g, line_color_b);  // 선분 색
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < pg->n_points; i++)  
		glVertex2f(pg->point[i][0], pg->point[i][1]);
	glEnd();
}

void update_center_of_gravity(My_Polygon* pg) {
	pg->center_x = pg->center_y = 0.0f;
	if (pg->n_points == 0) return;
	for (int i = 0; i < pg->n_points; i++) {
		pg->center_x += pg->point[i][0], pg->center_y += pg->point[i][1];
	}
	pg->center_x /= (float)pg->n_points, pg->center_y /= (float)pg->n_points;
}

// 2D 아핀 행렬을 이동, 확대/축소, 회전하기 위해 새로 추가된 함수들
void affine_move(Mat3x3* M, float t_x, float t_y) {
	M->mat3[0][2] = t_x;
	M->mat3[1][2] = t_y;
}

void affine_scale(Mat3x3* M, float s_x, float s_y) {
	M->mat3[0][0] = s_x;
	M->mat3[1][1] = s_y;
}

// 단위는 deg
void affine_rotate(Mat3x3* M, float theta) {
	const float PI = 3.14159265f;
	float rad = theta / 180.0f * PI;
	M->mat3[0][0] = cosf(rad);
	M->mat3[0][1] = -sinf(rad);
	M->mat3[1][0] = sinf(rad);
	M->mat3[1][1] = cosf(rad);
}

// 2D 아핀 행렬의 합성
Mat3x3 affine_combine(Mat3x3 M1, Mat3x3 M2) {
	Mat3x3 res;
	res.mat3[0][0] -= 1.0f, res.mat3[1][1] -= 1.0f;
	for (int i = 0; i < 2; i++) {  // 2D 아핀 변환 행렬 간의 곱의 3번째 행은 0, 0, 1로 고정
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				res.mat3[i][j] += M1.mat3[i][k] * M2.mat3[k][j];
			}
		}
	}
	return res;
}

// 주어진 2D 아핀 행렬과 다각형의 모든 좌표의 행렬 곱셈
My_Polygon* affine_vec_cal(My_Polygon* pg, Mat3x3* M) {
	for (int i = 0; i < pg->n_points; i++) {
		float temp[3] = { pg->point[i][0], pg->point[i][1], 1 };  // 현재 좌표 (x, y, 1)T
		float res[2] = { 0.0f, 0.0f };  // 변환된 좌표 (x', y', 1)T 
		for (int j = 0; j < 2; j++) {  // 1은 필요없으므로 계산에서 생략
			for (int k = 0; k < 3; k++) {
				res[j] += M->mat3[j][k] * temp[k];
			}
			pg->point[i][j] = res[j];  // 계산 결과 대입
		}
	}
	return pg;
}

// 다각형의 모든 점과 무게 중심을 각각 del_x, del_y만큼 이동
void affine_move_polygon(My_Polygon* pg, float del_x, float del_y) {
	Mat3x3 M;
	affine_move(&M, del_x, del_y);  // 다각형의 모든 점의 좌표를 이동시킨 후
	update_center_of_gravity(affine_vec_cal(pg, &M));  // 무게 중심까지 이동
}

// 무게 중심을 중심으로 theta만큼 회전하려면:
// 무게 중심을 원점으로 이동 -> 원점을 기준으로 회전 -> 원점에서 무게 중심으로 복귀
void affine_rotate_gravity_point(My_Polygon* pg, float theta) {
	Mat3x3 R, M1, M2, res;
	affine_move(&M1, -pg->center_x, -pg->center_y);
	affine_rotate(&R, theta);
	affine_move(&M2, pg->center_x, pg->center_y);
	res = affine_combine(R, M1), res = affine_combine(M2, res);
	affine_vec_cal(pg, &res);
}

// 무게 중심을 중심으로 x축은 f1만큼, y축은 f2만큼 확대/축소하려면:
// 무게 중심을 원점으로 이동 -> 원점을 기준으로 확대/축소 -> 원점에서 무게 중심으로 복귀
void affine_scale_gravity_point(My_Polygon* pg, float f1, float f2) {
	Mat3x3 S, M1, M2, res;
	affine_move(&M1, -pg->center_x, -pg->center_y);
	affine_scale(&S, f1, f2);
	affine_move(&M2, pg->center_x, pg->center_y);
	res = affine_combine(S, M1), res = affine_combine(M2, res);
	affine_vec_cal(pg, &res);
}

// 무게 중심을 중심으로 확대/축소 및 회전을 동시에
void affine_scale_and_rotate_gravity_point(My_Polygon* pg, float f1, float f2, float theta) {
	Mat3x3 S, R, M1, M2, res;
	affine_move(&M1, -pg->center_x, -pg->center_y);
	affine_scale(&S, f1, f2);
	affine_rotate(&R, theta);
	affine_move(&M2, pg->center_x, pg->center_y);
	res = affine_combine(S, M1), res = affine_combine(R, res), res = affine_combine(M2, res);
	affine_vec_cal(pg, &res);
}

/////////////////////////////////////////////////////////////////////////////////

/*void move_points(My_Polygon* pg, float del_x, float del_y) {
	for (int i = 0; i < pg->n_points; i++) {
		pg->point[i][0] += del_x, pg->point[i][1] += del_y;
	}
}

void rotate_points_around_center_of_grivity(My_Polygon* pg) {
	for (int i = 0; i < pg->n_points; i++) {
		float x, y;
		x = COS_5_DEGREES * (pg->point[i][0] - pg->center_x)
			- SIN_5_DEGREES * (pg->point[i][1] - pg->center_y) + pg->center_x;
		y = SIN_5_DEGREES * (pg->point[i][0] - pg->center_x)
			+ COS_5_DEGREES * (pg->point[i][1] - pg->center_y) + pg->center_y;
		pg->point[i][0] = x, pg->point[i][1] = y;
	}
}

void scale_points_around_center_of_grivity(My_Polygon* pg, float scale_factor) {
	for (int i = 0; i < pg->n_points; i++) {
		float x, y;
		x = scale_factor * (pg->point[i][0] - pg->center_x)
			 + pg->center_x;
		y = scale_factor * (pg->point[i][1] - pg->center_y)
			+ pg->center_y;
		pg->point[i][0] = x, pg->point[i][1] = y;
	}
}*/
