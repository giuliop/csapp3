(function (exports) {

    var px_h = 600,
        px_w = 1024,
        cell_size = 4;
        tick_time = 100,
        COLOR = '#ff4040';

    var cvs = document.createElement('canvas');
    document.body.appendChild(cvs);
    cvs.height = px_h;
    cvs.width = px_w;
    cvs.id = 'canvas';

    var ctx = cvs.getContext('2d'),
        h = cvs.height / cell_size,
        w = cvs.width / cell_size,
        interval_id,
        living = {};

    function next_gen() {
        var new_living = {},
            deads = {};
        for (var cell in living) {
            var nb = living_neighbors(parseInt(cell)).length;
            if ((nb > 1) && (nb < 4)) {
                new_living[cell] = true;
            }
            add_deads(parseInt(cell), deads);
        }
        check_dead(deads, new_living);
        living = new_living;
    }
    function neighbors(cell) {
        var nb = [],
        // world is "round"
            left = (cell % w == 0 ? w - 1 : -1),
            right = (cell % w == w - 1 ? -w + 1 : 1),
            top = (cell < w ? w * (h - 1) : -w),
            bottom = (cell >= w * (h-1) ? -w * (h-1) : w)
        ;
        nb.push(cell + top + left);
        nb.push(cell + top);
        nb.push(cell + top + right);
        nb.push(cell + bottom + left);
        nb.push(cell + bottom);
        nb.push(cell + bottom + right);
        nb.push(cell + left);
        nb.push(cell + right);
        return nb;
    }
    function living_neighbors(cell) {
        var cells = [];
        var nb = neighbors(cell);
        for (var c = 0; c < nb.length; c++) {
            if (living[nb[c]]) {
                cells.push(nb[c]);
            }
        }
        return cells;
    }
    function add_deads(cell, deads) {
        var nb = neighbors(cell);
        for (var c = 0; c < nb.length; c++) {
            if (!living[nb[c]] && !deads[nb[c]]) {
                deads[nb[c]] = true;
            }
        }
    }
    function check_dead(deads, alive) {
        for (var d in deads) {
            if (living_neighbors(parseInt(d)).length == 3) {
                alive[d] = true;
            }
        }
    }
    function draw() {
        ctx.fillStyle = COLOR;
        ctx.clearRect(0, 0, px_w, px_h);
        for (cell in living) {
            ctx.fillRect((cell % w) * cell_size, Math.floor(cell / w) * cell_size, cell_size, cell_size);
        }
    }
    function tick() {
        draw();
        next_gen();
        console.log('tick');
    }
    function start() {
        if (Object.keys(living).length == 0) {
            init()
        }
        interval_id = window.setInterval(tick, tick_time);
    }
    function stop() {
        window.clearInterval(interval_id);
    }
    function init() {
        //var i = w * 5 + w/2;
        //living = {};
        //living[i-w] = true;
        //living[i-2] = true;
        //living[i] = true;
        //living[i+w-1] = true;
        //living[i+w] = true;
        for (var i = 0; i < w * h; i++) {
            if (Math.random() > 0.95) {
                living[i] = true;
            }
        }
    }
    exports.start = start;
    exports.stop = stop;
})(this.life = {});

life.start();
