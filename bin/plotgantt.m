function [] = plotgantt(T, numMachines)
    distance = 1.1;
    mov_top = 1:length(T);
    ind=1:length(T);

    for i=1:length(T)
        I=ind(i);
        mov_top(I) = T(I).Machine - 1;
    end

    mov_top_max = zeros(1,length(mov_top)); %init
    for imov_top = 1:length(mov_top)
        mov_top_max(imov_top) = max(mov_top(imov_top));
    end
    mov_top_max = max(mov_top_max);
    mov_top_max_rotation = -1;

    for i=1:length(T)
        I=ind(i);
        plotblock(T(i),(mov_top_max+mov_top_max_rotation*mov_top(I))*distance);
    end

    % osie
    axis auto;
    ax = axis;

    minx = ax(1)-1;
    maxx = ax(2)+1;
    maxy = numMachines;
    axis([minx maxx ax(3) distance*maxy]);

    % kreski poziomie --
    for I=1:maxy
       handle = plot([minx-1 maxx+1],[(I-1)*distance (I-1)*distance],'k:');
    end

    set (get(handle,'Parent'),'YTickMode','manual');
    set (get(handle,'Parent'),'YTick',(distance/2):distance:(distance*maxy));

    procname = cell(1,3);
    for i = 1:maxy
        procname{i}=strcat('Maszyna ',num2str(maxy-i+1));
    end

    set (get(handle,'Parent'),'YTickLabel',procname);
end
