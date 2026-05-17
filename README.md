# 🌊 Interactive Underwater Ecosystem Visualization

A real-time 3D underwater ecosystem simulation developed using **C++**, **OpenGL**, and **FreeGLUT** as a Computer Graphics Lab Final Project.

This project creates an immersive underwater environment featuring animated marine life, dynamic environmental effects, interactive camera systems, and realistic ocean-inspired visuals.

---

# 📌 Project Overview

The simulation represents a procedurally generated underwater ecosystem where multiple aquatic elements interact in a visually rich environment. The project focuses on applying core computer graphics concepts including:

- Real-time rendering
- 3D transformations
- Lighting and materials
- Animation systems
- Procedural scene generation
- Fog and environmental effects
- Interactive camera controls

---

# ✨ Features

## 🐟 Marine Life Simulation
- Multiple animated fish species
- Realistic swimming movement
- Smooth directional movement system
- Fish panic behavior during shark mode
- Animated prawns and crabs

## 🦈 Shark Mode
- Large animated shark entity
- Dynamic predator behavior
- Fish panic and escape simulation
- Sound-triggered environment reaction

## 🌿 Underwater Vegetation
- Procedurally generated sea grass
- Smooth floating underwater movement
- Organic plant clustering
- Dynamic sway animation

## 🪸 Coral and Rock Generation
- Randomized coral structures
- Multi-colored coral variations
- Procedural rock placement
- Ocean floor detailing

## 💧 Water Environment Effects
- Dynamic underwater fog
- Semi-transparent water surface
- Floating plankton particles
- Bubble particle system
- Day and night lighting modes

## 🎥 Interactive Camera System
- Free camera mode
- Fish-follow camera mode
- Top-down observation mode
- Mouse-controlled rotation and zoom

## 🔊 Audio Integration
- Ambient underwater sound
- Bubble sound effects
- Shark activation effects
- Sonar-style audio feedback

---

# 🛠️ Technologies Used

- **C++**
- **OpenGL**
- **FreeGLUT**
- **GLU**
- **Windows Multimedia API (WinMM)**

---

# 🎮 Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move Camera |
| Arrow Keys | Rotate View |
| Mouse Left | Rotate Camera |
| Mouse Right | Zoom Camera |
| B | Toggle Bubbles |
| F | Toggle Fog |
| N | Toggle Day/Night |
| C | Change Camera Mode |
| P / Space | Pause Animation |
| R | Reset Scene |
| S | Toggle Shark Mode |
| ESC | Exit Application |

---

# 🧠 Computer Graphics Concepts Implemented

- 3D Object Modeling
- Real-Time Animation
- Transformation Matrices
- Perspective Projection
- Lighting and Shading
- Material Properties
- Procedural Generation
- Fog Effects
- Particle Systems
- Camera Systems
- Event Handling

---

# ⚙️ Compilation

## Windows (MinGW)

```bash
g++ OceanUnderwater.cpp -o OceanUnderwater.exe ^
-lfreeglut -lopengl32 -lglu32 -lwinmm -std=c++17
```

## Linux

```bash
g++ OceanUnderwater.cpp -o OceanUnderwater \
-lGL -lGLU -lglut -std=c++17
```

---

# 📷 Project Highlights

- Fully interactive underwater ecosystem
- Real-time animated marine life
- Dynamic environmental effects
- Multi-mode camera system
- Procedurally generated scene elements
- Integrated audio feedback system

---

# 📚 Academic Context

This project was developed as part of a **Computer Graphics Laboratory Final Project**, focusing on practical implementation of OpenGL-based real-time rendering and interactive graphics programming.

---

# 👨‍💻 Developer

Developed by **Talha Jobayer**

---
