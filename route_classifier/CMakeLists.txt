include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(route_classifier SHARED route_classifier.c)
target_link_libraries(route_classifier log_manager query_classifier)
install(TARGETS route_classifier DESTINATION lib)
