plugins {
    application
    kotlin("jvm") version "2.2.0"
    kotlin("plugin.serialization") version "1.8.0"
}

repositories {
    mavenCentral()
}

dependencies {
    testImplementation(kotlin("test"))
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
    mainClass = "client.ClientKt"
}
