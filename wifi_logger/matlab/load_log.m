%  Copyright (C) 2012, Robert Tang <opensource@robotang.co.nz>
%
%  This is free software; you can redistribute it and/or
%  modify it under the terms of the GNU Lesser General Public
%  License as published by the Free Software Foundation; either
%  version 2.1 of the License, or (at your option) any later version.
%
%  This program is distributed in the hope that it will be useful,
%  but WITHOUT ANY WARRANTY; without even the implied warranty of
%  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
%  Lesser General Public License for more details.
%
%  You should have received a copy of the GNU Lesser General Public Licence
%  along with this library; if not, see <http://www.gnu.org/licenses/>.

function [x y quality strength essid] = load_log(file, raw_nmea)
    if raw_nmea
        [lat long] = load_nmea(file);
        quality = zeros(size(lat));
        strength = zeros(size(lat));
        essid = zeros(size(lat));
    else
        ; % TODO
    end

    [x y] = latlong_to_xy(lat, long, 1);
    %[x y] = latlong_to_xy2(lat, long, 1);
end

% Supporting functions

function [r] = deg2rad(x)
    r = x * pi/180;
end

function [lat long] = load_nmea(file)
    fid = fopen(file);
    i = 1;
    tline = fgets(fid);
    while ischar(tline)
        if strncmp(tline, '$GPRMC', 6) == 1
            A = sscanf(tline, '$GPRMC,%f,%c,%f,%c,%f,%c');
            if length(A) == 6 && A(2) == 65 % if complete GPS packet and fix is valid
                lat(i) = A(3);
                long(i) = A(5);
                i = i + 1;
            end
        end       
        
        tline = fgets(fid);
    end
    
    fclose(fid);
end

function [lat_degree_m lon_degree_m] = lat_long_deg_m(lat)
    lat_degree_m = 1109.7356; % Hardcoded for North Shore, New Zealand
    lon_degree_m = 892.6383; % Hardcoded for North Shore, New Zealand
end

function [x y] = latlong_to_xy(lat, long, ref)
    [lat_degree_m long_degree_m] = lat_long_deg_m(lat);
    x = (lat - lat(ref)) .* lat_degree_m;
    y = (long - long(ref)) .* long_degree_m;
end

function [x y] = latlong_to_xy2(lat, long, ref)
    R = 6367;
    x = 10*R*deg2rad(long - long(ref))*cos(deg2rad(lat(ref)));
    y = 10*R*deg2rad(lat - lat(ref));
    
    % swap coordinates to match up with latlong_to_xy (as ive ignored
    % direction for latitudes and longitudes
    tmp = x;
    x = y;
    y = tmp;
end
