# Development container for pam_oauth2_device.
#
# Based on AlmaLinux to stay close to the platforms the module is actually
# packaged for (see packaging/rpm).  Override with --build-arg IMAGE_BASE to
# develop against a different EL release.
ARG IMAGE_BASE=quay.io/almalinux/almalinux:9

FROM $IMAGE_BASE

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN dnf install -y epel-release \
 && dnf install -y \
    ca-certificates \
    clang-tools-extra \
    diffutils \
    gcc \
    gcc-c++ \
    gdb \
    git \
    gtest-devel \
    less \
    libcurl-devel \
    make \
    openldap-devel \
    openssh-clients \
    pam-devel \
    pamtester \
    procps-ng \
    python3 \
    rpm-build \
    rpmlint \
    sudo \
    valgrind \
    which \
 && dnf clean all \
 && rm -rf /var/cache/dnf

# Unprivileged user matching the typical host UID so bind-mounted sources stay
# writable.
RUN groupadd --gid $USER_GID $USERNAME \
 && useradd --uid $USER_UID --gid $USER_GID --create-home --shell /bin/bash $USERNAME \
 && echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
 && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME
WORKDIR /workspaces/pam_oauth2_device
