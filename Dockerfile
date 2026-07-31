# Persona 3 Dual - Developer Environment
#
# Based on the official devkitPro image so devkitARM is already set up.
# Adds nds-dev (libnds + NDS toolchain), ffmpeg, mtools, and Python deps.
#
# Lint Dockerfile with: docker run --rm -i hadolint/hadolint < Dockerfile
#
# Build:  docker build -t p3d-dev-dev .
# Use:    docker run --rm -it -v "$(pwd)":/project p3d-dev-dev
# Make:   docker run --rm -v "$(pwd)":/project p3d-dev-dev make

FROM devkitpro/devkitarm:20260610

LABEL maintainer="P3D Team"
LABEL description="Full build environment for Persona 3 Dual (NDS homebrew)"

# Suppress interactive apt prompts
ENV DEBIAN_FRONTEND=noninteractive

# System packages
# ffmpeg        – video/audio asset conversion (used by the asset pipeline)
# mtools        – FAT image creation (sdcard.img)
# libblas3      – required by ffmpeg at runtime (update-alternatives symlink)
# liblapack3    – same as above
# python3 / pip – asset pipeline scripts
# zip / gzip    – packaging release artifacts
# git-lfs       – large file storage (LFS pointers resolved during CI checkout)
# ccache        – compiler cache for faster rebuilds (CI manages cache via actions/cache)
# gdb-multiarch - debugger
RUN apt-get update && apt-get install -y --no-install-recommends \
    ffmpeg=7:5.1.9-0+deb12u1 \
    mtools=4.0.33-1+really4.0.32-1 \
    libblas3=3.11.0-2 \
    liblapack3=3.11.0-2 \
    python3=3.11.2-1+b1 \
    python3-pip=23.0.1+dfsg-1 \
    python3-venv=3.11.2-1+b1 \
    zip=3.0-13 \
    gzip=1.12-1 \
    git-lfs=3.3.0-1+deb12u1 \
    ccache=4.8+really4.7.5-1 \
    gdb-multiarch=13.1-3 \
    && git lfs install --system \
    && rm -rf /var/lib/apt/lists/*

# NDS toolchain
# devkitARM is already in the base image; nds-dev adds the NDS-specific libs.
RUN dkp-pacman -Syu --noconfirm nds-dev

# Python virtual environment
# The Makefile calls /root/.venv/bin/python3 directly (matching the GitHub
# Actions workflow which creates ~/.venv on the runner, resolving to /root/.venv).
# We create the venv at that exact path so both environments resolve the same way.
COPY tools/requirements.txt /tmp/requirements.txt
RUN python3 -m venv /root/.venv \
    && /root/.venv/bin/pip install --no-cache-dir --upgrade pip \
    && /root/.venv/bin/pip install --no-cache-dir -r /tmp/requirements.txt

# Put the venv on PATH so bare `python3` and `pip` also work interactively.
ENV PATH="/root/.venv/bin:$PATH"

# Working directory
# Mount your repo here:  -v "$(pwd)":/project
WORKDIR /project

# Add aigis user so we don't run as root on dev container
RUN useradd -m aigis \
    && chown -R aigis:aigis /opt/devkitpro

# Give sudo access to aigis
RUN echo "aigis ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/aigis \
    && chmod 0440 /etc/sudoers.d/aigis \
    && visudo -c -f /etc/sudoers.d/aigis \
    && rm -rf /var/lib/apt/lists/*

USER aigis

# venv for aigis
RUN python3 -m venv "$HOME/.venv" \
    && "$HOME/.venv/bin/pip" install --upgrade pip \
    && "$HOME/.venv/bin/pip" install -r /tmp/requirements.txt
ENV PATH="/home/aigis/.venv/bin:$PATH"

# Default: drop into a shell so developers can run make, explore, debug, etc.
CMD ["/bin/bash"]
