function [] = plotblock(T, y0)
    % procedure
    color = T.Color;
    start = T.StartTime;
    finish = T.StartTime + T.ProcTime;
    
    y1 = y0+0.75;
    y1m = y0+0.9;

    % blok
    fill([start finish finish start], [y0 y0 y1 y1], color);
    hold on;
    
    % czas rozpoczecia
    handle=plot([start start],[y0 y1m],'k^-');
    set(handle,'MarkerFaceColor',color);

    % tytul bloczka
    hand_text = text((0.2 + start),y0+0.15,T.Name);
    set(hand_text,'FontWeight','normal');
    set(hand_text,'FontSize',12);

    % os x
    axis auto;
    ax = axis;
    axis([ax(1)-1 ax(2)+1 ax(3) ax(4)+0.5]);

    % tytul osi x
    xlabel('t');

    % wylacz os y
    set(get(handle,'Parent'),'YTickMode','manual');
    set(get(handle,'Parent'),'YTick',[]);
end

