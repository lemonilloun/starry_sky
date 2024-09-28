function out = transform(S_0)
% Функция для перехода из геоцентрической системы координат в связанную
% S_0 - координаты вектора в геоцентрической СК

global omega i w_pi nu theta gamma psi
% Переход из неподвижной геоцентрической СК в геоцентрическую орбиательную СК
% M_1 - матрица перехода из неподвижной геоцентрической СК в геоцентрическую орбитальную СК
M_1(1,1) = cos(w_pi)*cos(omega) - sin(w_pi)*sin(omega)*cos(i);
M_1(1,2) = cos(w_pi)*sin(omega) + sin(w_pi)*cos(omega)*cos(i);
M_1(1,3) = sin(w_pi)*sin(i);
M_1(2,1) = -sin(w_pi)*cos(omega) - cos(w_pi)*sin(omega)*cos(i);
M_1(2,2) = -sin(w_pi)*sin(omega) + cos(w_pi)*cos(omega)*cos(i);
M_1(2,3) = cos(w_pi)*sin(i);
M_1(3,1) = sin(omega)*sin(i);
M_1(3,2) = -cos(omega)*sin(i);
M_1(3,3) = cos(i);
S_1 = M_1 * S_0;

% Переход из геоцентрической орбитальной СК в барицентрическую орбитальную СК
% M_2 - матрица перехода из геоцентрической орбитальной СК в барицентрическую орбитальную СК
nu = 0; % Угол истинной аномалии
M_2 = [cos(nu), sin(nu), 0;...
       -sin(nu), cos(nu), 0;...
        0,       0,       1];
S_2 = M_2 * S_1;

% Переход из барицентрической орбитальной СК в связанную СК
% M_3 - матрица перехода из барицентрической орбитальной СК в связанную СК
M_3 = [cos(theta)*cos(psi),                                 sin(theta),            -cos(theta)*sin(psi);...
       sin(gamma)*sin(psi)-cos(gamma)*sin(theta)*cos(psi),  cos(gamma)*cos(theta), sin(gamma)*cos(psi)+cos(gamma)*sin(theta)*sin(psi);...
       cos(gamma)*sin(psi)+sin(gamma)*sin(theta)*cos(psi), -sin(gamma)*cos(theta), cos(gamma)*cos(psi)-sin(gamma)*sin(theta)*sin(psi)];
out = M_3 * S_2;
end