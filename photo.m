function [] = photo(RA,Dec,T,angel_m)
% [Width Height] - matrix resolution in pixels
Resolution = [1280 1038];  % CubeStar
pix = 16/1024; % величина пиксела в мм
MatrixSize = Resolution * pix;
Magnitude = 3.8;   % CubeStar % видимая звездная величина
FoV = [58 47]; % Field of View CubeStar

% % [Width Heigt] - matrix size in mm, 
% MatrixSize = [16 16]; % БОКЗ-М60/1000
% % Focal length in mm
% Focus = 60; % БОКЗ-М60/1000
% Resolution = [1024 1024]; % БОКЗ-М60/1000
% Magnitude = 4.7; % БОКЗ-М60/1000

load('C.mat') % звездный каталог
[n,m] = size(C);
Mag = Magnitude;

N = length(RA);
for q = 1 : 1 : N
    j = 1;
    for i = 1 : 1 : n
        if isequal(class(C{i,2}),'double') && C{i,14} <= Mag % выборка звезд, удовлетворяющих звездной величине
            HIP(j,:) = [C{i,2},C{i,8}*15*pi/180,C{i,9}*pi/180];  % Номер звезды, прямое восхождение (рад), склонение (рад)      
            [h, a] = transformation(Dec(q), RA(q), HIP(j,2), HIP(j,3)); % функция перевода в горизонтальные координаты
            szHIP(j,:) = C{i,14}; % звездная величина
            H(j,1) = h; % высота звезды над горинтом
            A(j,1) = a; % азимут звезды
            j = j + 1;
        end
    end
    szHIP = -szHIP;
    szHIP = (szHIP + abs(min(szHIP)) + 1);

    % AngleX = atan(MatrixSize(1)/(Focus*2));
    % AngleY = atan(MatrixSize(2)/(Focus*2));

    AngleX = (FoV(1) / 2) * pi / 180; % угол от центра кадра до левой (правой) границы кадра
    AngleY = (FoV(2) / 2) * pi / 180; % угол от центра кадра до верхней (нижней) границы кадра
    x = cos(H(H>=0)).*sin(A(H>=0))./sin(H(H>=0)); % ksi % выборка звезд, лежащих выше плоскости горизонта
    y = (-cos(H(H>=0)).*cos(A(H>=0)))./sin(H(H>=0)); % eta % выборка звезд, лежащих выше плоскости горизонта
    
    % Нахождение координат ksi и eta границ кадра
    lx = abs((sin(pi/2+AngleX)*cos(pi/2)-cos(pi/2+AngleX).*sin(pi/2).*cos(0-0))./(sin(pi/2)*sin(pi/2+AngleX)+cos(pi/2+AngleX).*cos(pi/2).*cos(0-0)));
    ly = abs((sin(pi/2+AngleY)*cos(pi/2)-cos(pi/2+AngleY).*sin(pi/2).*cos(0-0))./(sin(pi/2)*sin(pi/2+AngleY)+cos(pi/2+AngleY).*cos(pi/2).*cos(0-0)));
    
    % не обращать внимания на эти три строчки
    N_HIP = HIP(H>=0, 1); 
    RA_HIP = HIP(H>=0, 2);
    Dec_HIP = HIP(H>=0, 3);

    % Выборка звезд, которые лежат в центре кадра
    w1 = (x >= -lx);
    w2 = (x <= lx);
    w3 = (y >= -ly);
    w4 = (y <= ly);
    w = w1 .* w2 .* w3 .* w4;
    w = cast(w,'logical');

    % не обращать внимания на эти три строчки
    N_HIP = N_HIP(w);
    RA_HIP = RA_HIP(w);
    Dec_HIP = Dec_HIP(w);

    % Отбор звезд по булевым значениям
    X_w = x(w);
    Y_w = y(w);
    
    % на это смотреть не надо 
    sz = [0,0,(Resolution)];
    dpi = min(sz(3:4)./(MatrixSize/25.4));
    fig1 = figure;
    s = scatter(x(w),y(w),szHIP(w)*0.01);
    set(gca,'XColor', 'none','YColor','none','Color',[0 0 0],'Units','inches','Position', sz/dpi)
    set(gcf,'Units','inches','Visible','off','Position', sz/dpi); 
    s.MarkerFaceColor = [1 1 1];
    s.MarkerEdgeColor = [1 1 1];
    xlim([-lx lx])
    ylim([-ly ly])
    print(fig1,'-dpng',strcat('-r',num2str(int64(dpi))),strcat('img/photo_',num2str(q)))
    E = imread(strcat('img/photo_',num2str(q),'.png'));
    E(:,(Resolution(1)+1:end),:) = [];
    E = imcomplement(E);
    imwrite(E,strcat('img/photo_',num2str(q),'.png'))
    sphere_coor = [RA(q),Dec(q)];
    save(strcat('data/NN_HIP_',num2str(q),'.mat'),'N_HIP','X_w','Y_w','RA_HIP','Dec_HIP','sphere_coor')
    cla
end
end