# Runtime image for person-service.
#
# The binary is built by CI on the same Ubuntu 24.04 (`cmake --build --preset release`) and only packaged here:
# building userver inside `docker build` would take 20+ minutes on every run without a compiler cache.
FROM ubuntu:24.04

# Shared libraries the release binary links against (see `ldd person-service`).
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        libatomic1 \
        libbz2-1.0 \
        libcctz2 \
        libcurl4t64 \
        libfmt9 \
        libgssapi-krb5-2 \
        libicu74 \
        libldap2 \
        liblzma5 \
        libnghttp2-14 \
        libsasl2-2 \
        libssl3t64 \
        libstdc++6 \
        libzstd1 \
        zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY build/release/person-service/person-service /usr/local/bin/person-service
COPY person-service/configs/static_config.yaml /etc/person-service/static_config.yaml

USER nobody

# The listening port comes from $PORT (set by Render), 8080 by default.
EXPOSE 8080
CMD ["person-service", "--config", "/etc/person-service/static_config.yaml"]
