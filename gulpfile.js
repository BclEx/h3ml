'use strict';
const
    gulp = require('gulp'),
    gulp_concat = require('gulp-concat'),
    gulp_rename = require('gulp-rename'),
    gulp_uglify = require('gulp-uglify'),
    webpack = require('webpack'),
    webpackStream = require('webpack-stream'),
    express = require('express'),
    spawn = require('child_process').spawn;

gulp.task('webpack', function () {
    var r = gulp.src('./src.js/app.js')
        .pipe(webpackStream({
            output: {
                filename: 'app.bundle.js',
            },
            watch: webpackWatch,
            module: {
                loaders: [{
                    test: /\.js$/, loader: 'babel-loader',
                    exclude: /(node_modules)/,
                    query: {
                        //presets: ['es2015', 'react'],
                        plugins: ['transform-object-rest-spread']
                    }
                }]
            },
            plugins: [
                //new webpack.IgnorePlugin(/svg4everybody/),
            ],
            stats: { colors: true },
            devtool: 'source-map'
        }))
        .pipe(gulp.dest('./www/assets/'));
});

var webpackWatch = false;
gulp.task('webpack:watch', function () {
    webpackWatch = true;
});

gulp.task('default', ['webpack:watch', 'webpack'], function () {
});

gulp.task('lib', function () {
    var r = gulp.src([
        './src.js/lib.js'])
        .pipe(gulp_concat('lib.bundle.js'))
        .pipe(gulp.dest('./www/assets/'));
});

gulp.task('express', function () {
    // express
    var app = express();
    app.set('port', process.env.PORT || 8080);
    app.set('view engine', 'ejs');
    app.use(express.static('www'));

    // start server
    app.listen(app.get('port'), function () {
        console.log('Node app is running on port', app.get('port'));
    });
});

gulp.task('serve', function () {
    spawn('node', ['node_modules/gulp/bin/gulp.js', 'express', 'default'], { stdio: 'inherit' });
});
