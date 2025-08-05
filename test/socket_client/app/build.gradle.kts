plugins {
    application
    kotlin("jvm") version "2.0.0"
    kotlin("plugin.serialization") version "1.8.0"
}

/*
buildscript {
    ext {
        kotlin_ver = "2.1.10"
        // compose_compiler_ver = "1.3.0"
        // compose_ver = "1.2.1"
    }
}
*/

repositories {
    mavenCentral()
}

dependencies {
    testImplementation(kotlin("test"))
    implementation ("com.squareup.okhttp3:okhttp:4.9.3")
    implementation("com.google.code.gson:gson:2.11.0")
    implementation("com.github.ajalt.clikt:clikt:5.0.3")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.8.0")
}

tasks.test {
    useJUnitPlatform()
}

kotlin {
    jvmToolchain(21)
}

application {
    mainClass = "networking.MainKt"
}
