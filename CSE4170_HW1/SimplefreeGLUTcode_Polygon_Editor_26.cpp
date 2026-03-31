#include <stdio.h>
#include <math.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "Definitions_26.h"

Window wd;
Status st;
My_Polygon pg;

/*
pgs는 다각형을 최대 MAX_NUMBER_OF_POLYGONS개까지 저장하는 배열
pg_exists는 해당 index의 pgs에 다각형 저장 여부를 판단하는 bool 배열
pg_exists[idx]가 true인 pgs[idx]에만 접근 허용, 반드시 if문으로 거칠 것
pg_num은 현재 존재하는 다각형의 개수
selected_pg는 선택된 pg의 index를 저장, 없을 경우에는 -1
a_scale은 애니메이션 모드에서 확대면 1, 축소면 -1
rotate_deg는 회전 모드에서의 회전 각도
*/
My_Polygon pgs[MAX_NUMBER_OF_POLYGONS] = { 0, };
bool pg_exists[MAX_NUMBER_OF_POLYGONS] = { 0, };
int pg_num = 0, selected_pg = -1, a_scale = 1;
float rotate_deg;

// pg 변수 초기화
void clean_pg(My_Polygon* pg) {
	for (int i = 0; i < pg->n_points; i++)
		for (int j = 0; j < 2; j++)
			pg->point[i][j] = 0.0f;
	pg->n_points = 0;
	pg->center_x = 0.0f, pg->center_y = 0.0f;
	pg->animation_val = 0;
}

// 다각형을 배치할 남는 index 찾기
int find_empty_pgs() {
	for (int i = 0; i < MAX_NUMBER_OF_POLYGONS; i++)
		if (!pg_exists[i]) return i;
}

// 다각형 추가
void save_to_pgs(int idx) {
	pg_exists[idx] = true;
	pgs[idx] = pg;
	pg_num++;
}

// 다각형 제거
void remove_from_pgs(int idx) {
	clean_pg(&pgs[idx]);
	pg_exists[idx] = false;
	pg_num--;
}

// 무게 중심을 제대로 클릭했는지 판정하는 함수: 밖으로 빼냈음
bool click_check(float x_n, float y_n, int i) {
	return (fabs(x_n - pgs[i].center_x) < CENTER_SELECTION_SENSITIVITY) &&
		(fabs(y_n - pgs[i].center_y) * (float)wd.width / (float)wd.height < CENTER_SELECTION_SENSITIVITY);
}

// 선분 색깔
// RED, ORANGE, YELLOW, LIME, GREEN, CYAN, BLUE, DARKBLUE, MAGENTA, PURPLE
float line_color[MAX_NUMBER_OF_POLYGONS][3] = {
	{1.0f, 0.0f, 0.0f}, {1.0f, 165.0f / 255.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 0.0f}, {0.0f, 128.0f / 255.0f, 0.0f}, {0.0f, 1.0f, 1.0f},
	{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 139.0f / 255.0f}, {1.0f, 0.0f, 1.0f},
	{128.0f / 255.0f, 0.0f, 128.0f / 255.0f}
};

// GLUT callbacks
void timer(int value) {
	// 다각형 선택 모드에 진입한 순간의 크기를 기준으로 확대나 축소를 반복
	// 일정 횟수만큼 확대나 축소를 했으면 반대의 동작을 수행
	// 다각형 선택 모드에서 나가면 기준값 초기화
	float a_scale_factor;
	if (pgs[selected_pg].animation_val > 17 || pgs[selected_pg].animation_val < -17)
		a_scale = -a_scale;
	pgs[selected_pg].animation_val += a_scale;
	if (a_scale > 0) a_scale_factor = SCALE_UP_FACTOR;
	else a_scale_factor = SCALE_DOWN_FACTOR;

	affine_scale_and_rotate_gravity_point(&pgs[selected_pg], a_scale_factor, a_scale_factor, -5.0f);
	glutPostRedisplay();
	if (st.animation_mode)
		glutTimerFunc(ROTATION_STEP, timer, 0);
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	// 다각형 생성 모드에서 선분 긋기
	if (pg.n_points > 0) {
		if (st.create_mode)
			draw_lines_by_points(&pg, LINE_COLOR_SELECTED);
	}
	
	// 모든 다각형의 무게 중심 나타내기
	for (int i = 0; i < MAX_NUMBER_OF_POLYGONS; i++) {
		if (pg_exists[i]) {
			// 다각형 선분 그리기
			draw_lines_by_points(&pgs[i], line_color[i][0], line_color[i][1], line_color[i][2]);

			// 다각형이 선택된 상태인지 아닌지에 따라 무게 중심 색깔 변경
			if (st.select_mode || st.animation_mode) {
				if (i == selected_pg)
					glColor3f(CENTER_POINT_COLOR_SELECTED);
				else glColor3f(CENTER_POINT_COLOR);
			}
			else glColor3f(CENTER_POINT_COLOR);
			glBegin(GL_POINTS);
			glVertex2f(pgs[i].center_x, pgs[i].center_y);
			glEnd();
		}
	}
	// 더미 렌더링
	glBegin(GL_POINTS);
	glEnd();
	glFlush();
}

// 키보드로 모드 간 전환
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	// 대기 모드 -> 다각형 생성 모드
	case 'S':
	case 's':
		if (st.standby_mode) {
			if (pg_num < MAX_NUMBER_OF_POLYGONS) {  // 최대 다각형 수를 넘어가지 않는 선에서 다각형 생성
				st.standby_mode = 0;
				st.create_mode = 1;
				glClearColor(BACKGROUND_CREATE_COLOR, 1.0f);
				glutPostRedisplay();
				fprintf(stderr, "*** Successfully entered CREATE MODE!\n");
			}
			// 현재 존재하는 다각형이 최대 다각형 수를 넘어가면 다각형 생성 모드로 넘어가지 못함
			else
				fprintf(stderr, "*** Too many POLYGONS!\n");
		}
		break;
	// 다각형 생성 모드에서 점을 생성하고 다각형을 확정짓기
	case 'E':
	case 'e':
		if (st.create_mode) {
			// 점이 3개 이상이면 대기 모드로 전환
			if (pg.n_points >= 3) {
				st.create_mode = 0;
				st.standby_mode = 1;
				fprintf(stderr, "*** POLYGON selection is finished!\n");
				glClearColor(BACKGROUND_COLOR, 1.0f);
				glutPostRedisplay();
				update_center_of_gravity(&pg);
				pg.animation_val = 0;

				// 생성된 다각형을 배열 pgs에 저장
				int idx = find_empty_pgs();
				save_to_pgs(idx);
				clean_pg(&pg);
			}
			// 점이 3개 미만인데 다각형을 생성하려고 하면 허용하지 않고 계속 점을 생성하도록 함
			else
				fprintf(stderr, "*** Choose at least three points!\n");
		}
		break;
	// 다각형 선택 모드에서 c를 누르면 다각형 삭제 후 대기 모드로 전환
	case 'C':
	case 'c':
		if (st.select_mode) {
			st.select_mode = 0;
			st.standby_mode = 1;
			remove_from_pgs(selected_pg);
			selected_pg = -1;
			fprintf(stderr, "*** POLYGON successfully deleted!\n");
			glClearColor(BACKGROUND_COLOR, 1.0f);
			glutPostRedisplay();
		}
		break;
	// a를 누르면 다각형 선택 모드 <-> 애니메이션 모드 간 전환
	case 'A':
	case 'a':
		if (st.select_mode) {
			st.animation_mode = 1;
			st.select_mode = 0;
			fprintf(stderr, "*** Successfully entered ANIMATION MODE!\n");
			glClearColor(BACKGROUND_ANIMATION_COLOR, 1.0f);
			glutTimerFunc(ROTATION_STEP, timer, 0);
		}
		else if (st.animation_mode) {
			st.select_mode = 1;
			st.animation_mode = 0;
			fprintf(stderr, "*** Successfully reentered SELECTION MODE!\n");
			glClearColor(BACKGROUND_SELECT_COLOR, 1.0f);
			glutPostRedisplay();
		}
		break;
	}
}

float aspect_ratio;  // set in reshape callback
void wheel(int wheel, int direction, int x, int y) {
	if (st.select_mode) {
		if (direction == -1)  // 마우스를 아래로 스크롤: 확대
			affine_scale_gravity_point(&pgs[selected_pg], SCALE_UP_FACTOR, SCALE_UP_FACTOR);
		else  // 마우스를 위로 스크롤: 축소
			affine_scale_gravity_point(&pgs[selected_pg], SCALE_DOWN_FACTOR, SCALE_DOWN_FACTOR);
		glutPostRedisplay();
	}
}

// 다각형 선택 모드에서 좌우 방향키를 누르면 좌우 반전, 상하 방향키를 누르면 상하 반전
void special(int key, int x, int y) {
	if (st.select_mode && !st.move_mode && !st.rotation_mode) {
		switch (key) {
		case GLUT_KEY_LEFT:
		case GLUT_KEY_RIGHT:
			affine_scale_gravity_point(&pgs[selected_pg], -1.0f, 1.0f);
			glutPostRedisplay();
			break;
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			affine_scale_gravity_point(&pgs[selected_pg], 1.0f, -1.0f);
			glutPostRedisplay();
			break;
		}
	}
}


static int prev_x, prev_y;
void mousepress(int button, int state, int x, int y) {
	// 다각형 생성 모드에서 SHIFT + 좌클릭으로 점 생성
	if (st.create_mode) {
		if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {
			int key_state = glutGetModifiers();
			if (key_state & GLUT_ACTIVE_SHIFT) {
				// 다각형 생성시 꼭짓점 개수를 128개로 제한
				if (pg.n_points == MAX_NUMBER_OF_VERTICES_PER_POLYGON)
					fprintf(stderr, "*** Unable to add more points! Max # of points are %d\n", MAX_NUMBER_OF_VERTICES_PER_POLYGON);
				else {
					add_point(&pg, &wd, x, y);
					glutPostRedisplay();
				}
			}
		}
	}
	else {
		if (button == GLUT_LEFT_BUTTON) {
			if (state == GLUT_DOWN) {
				st.leftbuttonpressed = 1;  // 마우스 왼쪽 버튼이 눌려진 상태
				prev_x = x, prev_y = y;

				float x_normalized = 2.0f * ((float)x) / wd.width - 1.0f;
				float y_normalized = 2.0f * ((float)wd.height - y) / wd.height - 1.0f;
				for (int i = 0; i < MAX_NUMBER_OF_POLYGONS; i++) {
					if (pg_exists[i]) {
						// 대기 모드에서 다각형의 무게 중심 좌클릭시 다각형 선택 모드로 전환
						// 그 다각형은 "선택된" 상태
						if (st.standby_mode && click_check(x_normalized, y_normalized, i)) {
							st.standby_mode = 0;
							st.select_mode = 1;
							selected_pg = i;
							fprintf(stderr, "*** Selected POLYGON # is: %d\n", selected_pg);
							glClearColor(BACKGROUND_SELECT_COLOR, 1.0f);
							glutPostRedisplay();
						}
						// 다각형 선택 모드에서 좌클릭
						else if (st.select_mode) {
							// 현재 선택된 다각형의 무게 중심: 선택 해제 및 대기 모드로 전환
							// 애니메이션 모드 관련된 값들도 선택 해제시 초기화
							if (i == selected_pg && click_check(x_normalized, y_normalized, i)) {
								st.select_mode = 0;
								st.standby_mode = 1;
								pgs[selected_pg].animation_val = 0;
								selected_pg = -1;
								a_scale = 1;
								fprintf(stderr, "*** Successfully entered STANDBY MODE!\n");
								glClearColor(BACKGROUND_COLOR, 1.0f);
								glutPostRedisplay();
							}
							// 그 외의 장소: 마우스 왼쪽 버튼이 떼어질 때까지 이동 모드 유지 
							else {
								st.move_mode = 1;
							}
						}
					}
				}
				glutPostRedisplay();
			}
			else if (state == GLUT_UP) {
				st.leftbuttonpressed = 0;  // 마우스 왼쪽 버튼이 떼어진 상태
				st.move_mode = 0;
				glutPostRedisplay();
			}
		}
		// 마우스 오른쪽 버튼이 눌러졌을 때, 눌린 상태에서는 회전 모드 진입
		else if (button == GLUT_RIGHT_BUTTON) {
			if (state == GLUT_DOWN) {
				if (st.select_mode) {  // 다각형 선택 모드에서만 회전 모드 진입 가능
					prev_x = x, prev_y = y;
					st.rightbuttonpressed = 1;
					st.rotation_mode = 1;
					glutPostRedisplay();
				}
			}
			else if (state == GLUT_UP) {
				st.rightbuttonpressed = 0;
				st.rotation_mode = 0;
				glutPostRedisplay();
			}
		}
	}
}

// 다각형 선택 모드일 때 마우스 버튼을 누른 상태에서 동작 가능
void mousemove(int x, int y) {
	// 이동 모드: 왼쪽 마우스 버튼을 클릭한 상태로 움직여서 다각형 이동
	if (st.leftbuttonpressed && st.select_mode && st.move_mode) {
		float delx, dely;
		delx = 2.0f * ((float) x - prev_x) / wd.width;
		dely = 2.0f * ((float) prev_y - y) / wd.height;
		prev_x = x, prev_y = y;

		affine_move_polygon(&pgs[selected_pg], delx, dely);
		glutPostRedisplay();
	}
	// 회전 모드: 오른쪽 마우스 버튼을 클릭한 상태로 움직여서 다각형 회전
	else if (st.rightbuttonpressed && st.select_mode && st.rotation_mode) {
		float delx, dely;
		delx = 2.0f * ((float)x - prev_x) / wd.width;
		dely = 2.0f * ((float)prev_y - y) / wd.height;
		prev_x = x, prev_y = y;
		
		// delx와 비례하는 값을 회전 각도로 설정
		if (delx > 0) rotate_deg = -fabsf(delx * ROTATION_SENSITIVITY);
		else rotate_deg = fabsf(delx * ROTATION_SENSITIVITY);
		affine_rotate_gravity_point(&pgs[selected_pg], rotate_deg);
		glutPostRedisplay();
	}
}

// 창 크기 바꾸면 크기 출력
void reshape(int width, int height) {
	fprintf(stdout, "### The new window size is %dx%d.\n", width, height);
	wd.width = width, wd.height = height;
	glViewport(0, 0, wd.width, wd.height);
}

void close(void) {
	fprintf(stdout, "\n^^^ The control is at the close callback function now.\n\n");
}
// End of GLUT callbacks

// 최초 실행
void initialize_polygon_editor(void) {
	wd.width = 800, wd.height = 600, wd.initial_anchor_x = 500, wd.initial_anchor_y = 200;
	// 최초 실행시 모드 설정: 대기 모드
	st.standby_mode = 1, st.create_mode = 0, st.select_mode = 0;
	st.move_mode = 0, st.rotation_mode = 0, st.animation_mode = 0;
	st.leftbuttonpressed = 0, st.rightbuttonpressed = 0;
	pg.n_points = 0; pg.center_x = 0.0f; pg.center_y = 0.0f;
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseWheelFunc(wheel);
	glutSpecialFunc(special);
	glutMouseFunc(mousepress);
	glutMotionFunc(mousemove);
	glutReshapeFunc(reshape);
	glutCloseFunc(close);
}

void initialize_renderer(void) {
	register_callbacks();

	glPointSize(5.0);
	glClearColor(BACKGROUND_COLOR, 1.0f);
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = TRUE;
	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

// 인사말 출력
void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, " PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, " Originally coded for CSE4170 students\n");
	fprintf(stdout, "   of Dept. of Comp. Sci. & Eng., Sogang University.\n");
	fprintf(stdout, " Modified by SeJoon Kim for Sogang CSE4170 HW1\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

// MAIN
#define N_MESSAGE_LINES 4
int main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 SimplefreeGLUTcode_Polygon_Editor_HW1";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 's', 'e', 'c', 'a'",
		"    - Special keys used: LEFT, RIGHT, UP, DOWN",
		"    - Mouse used: L-click, R-click and move",
		"    - Other operations: window reshape"
	};

	glutInit(&argc, argv);
	initialize_polygon_editor();

	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE); // <-- Be sure to use this profile for this example code!
 //	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutInitDisplayMode(GLUT_RGBA);

	glutInitWindowSize(wd.width, wd.height);
	glutInitWindowPosition(wd.initial_anchor_x, wd.initial_anchor_y);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

   // glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_EXIT); // default
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	
	glutMainLoop();
	fprintf(stdout, "^^^ The control is at the end of main function now.\n\n");
	return 0;
}
