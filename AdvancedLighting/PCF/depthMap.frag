#version 450 core
void main() {
    // 其实这一行代码会默认执行，可以注释掉
    gl_FragDepth = gl_FragCoord.z;
}

